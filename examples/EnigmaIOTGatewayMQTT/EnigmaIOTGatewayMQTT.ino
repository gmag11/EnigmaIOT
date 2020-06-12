/**
  * @file EnigmaIOTGatewayMQTT.ino
  * @version 0.9.1
  * @date 28/05/2020
  * @author German Martin
  * @brief MQTT Gateway based on EnigmaIoT over ESP-NOW
  *
  * EnigmaIOT Gateway to connect nodes to MQTT broker
  */


#include <Arduino.h>

#define MQTT_MAX_PACKET_SIZE 2048
#include <PubSubClient.h>
#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif // SECURE_MQTT

#include <GwOutput_generic.h>
#include "GwOutput_mqtt.h"

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h> // Comment to compile for ESP8266
#include <Update.h>
#include <SPIFFS.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_tls.h"
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
//#include <ESPAsyncTCP.h> // Comment to compile for ESP32
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <SPI.h>
#endif // ESP32

#include <ArduinoOTA.h>

#include <CayenneLPP.h>
#include <FS.h>

#include <EnigmaIOTGateway.h>
#include <helperFunctions.h>
#include <debug.h>
#include <espnow_hal.h>
#include <Curve25519.h>
#include <ChaChaPoly.h>
#include <Poly1305.h>
#include <SHA256.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 5
#endif // BUILTIN_LED

#define BLUE_LED LED_BUILTIN
#define RED_LED LED_BUILTIN

#ifdef ESP32
TimerHandle_t connectionLedTimer;
#elif defined(ESP8266)
ETSTimer connectionLedTimer;
#endif // ESP32

const int connectionLed = LED_BUILTIN;
boolean connectionLedFlashing = false;

void flashConnectionLed (void* led) {
	//digitalWrite (*(int*)led, !digitalRead (*(int*)led));
	digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN));
}

void startConnectionFlash (int period) {
#ifdef ESP32
	if (!connectionLedFlashing) {
		connectionLedFlashing = true;
		connectionLedTimer = xTimerCreate ("led_flash", pdMS_TO_TICKS (period), pdTRUE, (void*)0, flashConnectionLed);
		xTimerStart (connectionLedTimer, 0);
	}
#elif defined (ESP8266)
	ets_timer_disarm (&connectionLedTimer);
	if (!connectionLedFlashing) {
		connectionLedFlashing = true;
		ets_timer_arm_new (&connectionLedTimer, period, true, true);
	}
#endif // ESP32
}

void stopConnectionFlash () {
#ifdef ESP32
	if (connectionLedFlashing) {
		connectionLedFlashing = false;
		xTimerStop (connectionLedTimer, 0);
		xTimerDelete (connectionLedTimer, 0);
	}
#elif defined(ESP8266)
	if (connectionLedFlashing) {
		connectionLedFlashing = false;
		ets_timer_disarm (&connectionLedTimer);
		digitalWrite (connectionLed, HIGH);
	}
#endif // ESP32
}

void arduinoOTAConfigure () {
	// Port defaults to 3232
	// ArduinoOTA.setPort(3232);

	// Hostname defaults to esp3232-[MAC]
	ArduinoOTA.setHostname(EnigmaIOTGateway.getNetworkName ());

	// No authentication by default
	ArduinoOTA.setPassword(EnigmaIOTGateway.getNetworkKey(true));

	// Password can be set with it's md5 value as well
	// MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
	// ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

	ArduinoOTA.onStart ([]() {
		String type;
		if (ArduinoOTA.getCommand () == U_FLASH)
			type = "sketch";
		else // U_SPIFFS
			type = "filesystem";

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Serial.println ("Start updating " + type);
	});
	ArduinoOTA.onEnd ([]() {
		Serial.println ("\nEnd");
	});
	ArduinoOTA.onProgress ([](unsigned int progress, unsigned int total) {
		static bool printed = false;
		unsigned int percent = progress / (total / 100);
		if (!(percent % 1)) {
			Serial.print ('.');
		}
		if (!(percent % 20) && !printed && percent != 0) {
			Serial.printf (" %d%%\n", percent);
			printed = true;
		} else if (percent % 20) {
			printed = false;
		}
		if (progress == total) {
			Serial.println ("OTA transfer finished");
		}
		//static bool printed = false;
		//int percent = progress / (total / 100);
		//if (!(percent % 10) && !printed) {
		//	Serial.printf (" %u%%\n", percent);
		//	printed = true;
		//} else if (percent % 10) {
		//	//Serial.print ('.');
		//	printed = false;
		//}
	});
	ArduinoOTA.onError ([](ota_error_t error) {
		Serial.printf ("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println ("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println ("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println ("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println ("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println ("End Failed");
	});

	ArduinoOTA.begin ();
}

void wifiManagerExit (boolean status) {
	GwOutput.configManagerExit (status);
}

void wifiManagerStarted () {
	GwOutput.configManagerStart (&EnigmaIOTGateway);
}

void processRxControlData (char* macStr, uint8_t* data, uint8_t length) {
	if (data) {
		GwOutput.outputControlSend (macStr, data, length);
	}
}

void processRxData (uint8_t* mac, uint8_t* buffer, uint8_t length, uint16_t lostMessages, bool control, gatewayPayloadEncoding_t payload_type) {
	uint8_t* addr = mac;
	size_t pld_size;
	const int PAYLOAD_SIZE = 1024; // Max MQTT payload in PubSubClient library normal operation.

	char payload[PAYLOAD_SIZE];

	char mac_str[ENIGMAIOT_ADDR_LEN*3];
	mac2str (addr, mac_str);
	if (control) {
		processRxControlData (mac_str, buffer, length);
		return;
	}
	//char* netName = EnigmaIOTGateway.getNetworkName ();
	if (payload_type == CAYENNELPP) {
		DEBUG_INFO ("CayenneLPP message");
		const int capacity = JSON_ARRAY_SIZE (25) + 25 * JSON_OBJECT_SIZE (4);
		DynamicJsonDocument jsonBuffer (capacity);
		JsonArray root = jsonBuffer.createNestedArray ();
		CayenneLPP cayennelpp (MAX_DATA_PAYLOAD_SIZE);

		cayennelpp.decode ((uint8_t*)buffer, length, root);
		uint8_t error = cayennelpp.getError ();
		if (error != LPP_ERROR_OK) {
			DEBUG_ERROR ("Error decoding CayenneLPP data: %d", error);
			return;
		}
		pld_size = serializeJson (root, payload, PAYLOAD_SIZE);
	} else if (payload_type == MSG_PACK) {
		DEBUG_INFO ("MsgPack message");
		const int capacity = JSON_ARRAY_SIZE (25) + 25 * JSON_OBJECT_SIZE (4);
		DynamicJsonDocument jsonBuffer (capacity);
		DeserializationError error = deserializeMsgPack (jsonBuffer, buffer, length);
		if (error != DeserializationError::Ok) {
			DEBUG_ERROR ("Error decoding MSG Pack data: %s", error.c_str ());
			return;
		}
		pld_size = serializeJson (jsonBuffer, payload, PAYLOAD_SIZE);
    } else if (payload_type == RAW) {
		DEBUG_INFO ("RAW message");
		if (length <= PAYLOAD_SIZE) {
			memcpy (payload, buffer, length);
			pld_size = length;
		} else { // This will not happen but may lead to errors in case of using another physical transport
			memcpy (payload, buffer, PAYLOAD_SIZE);
			pld_size = PAYLOAD_SIZE;
		}
	} 

	GwOutput.outputDataSend (mac_str, payload, pld_size);
	DEBUG_INFO ("Published data message from %s, length %d: %s, Encoding 0x%02X", mac_str, pld_size, payload, payload_type);
	if (lostMessages > 0) {
		pld_size = snprintf (payload, PAYLOAD_SIZE, "%u", lostMessages);
		GwOutput.outputDataSend (mac_str, payload, pld_size, GwOutput_data_type::lostmessages);
		DEBUG_INFO ("Published MQTT from %s: %s", mac_str, payload);
	}
	pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%.2f}",
							EnigmaIOTGateway.getPER ((uint8_t*)mac),
							EnigmaIOTGateway.getErrorPackets ((uint8_t*)mac),
							EnigmaIOTGateway.getTotalPackets ((uint8_t*)mac),
							EnigmaIOTGateway.getPacketsHour ((uint8_t*)mac));
	GwOutput.outputDataSend (mac_str, payload, pld_size, GwOutput_data_type::status);
	DEBUG_INFO ("Published MQTT from %s: %s", mac_str, payload);
}

void onDownlinkData (uint8_t* address, control_message_type_t msgType, char* data, unsigned int len){
	uint8_t *buffer;
	unsigned int bufferLen = len;
	gatewayPayloadEncoding_t encoding = ENIGMAIOT;


	DEBUG_INFO ("DL Message for " MACSTR ". Type 0x%02X", MAC2STR (address), msgType);
	DEBUG_DBG ("Data: %.*s", len, data);

	if (msgType == USERDATA_GET || msgType == USERDATA_SET) {
		const int capacity = JSON_ARRAY_SIZE (25) + 25 * JSON_OBJECT_SIZE (4);
		DynamicJsonDocument json (capacity);
		DeserializationError error = deserializeJson (json, data, len, DeserializationOption::NestingLimit (3));
		if (error == DeserializationError::Ok) {
			DEBUG_INFO ("JSON Message. Result %s", error.c_str ());
			bufferLen = measureMsgPack (json) + 1; // Add place for \0
			buffer = (uint8_t*)malloc (bufferLen);
			bufferLen = serializeMsgPack (json, (char*)buffer, bufferLen);
			encoding = MSG_PACK;
		} else {
			DEBUG_INFO ("Not JSON Message. Error %s", error.c_str ());
			bufferLen++; // Add place for \0
			buffer = (uint8_t*)malloc (bufferLen);
			sprintf ((char*)buffer, "%.*s", len, data);
			encoding = RAW;
		}
	} else {
		bufferLen = len + 1; 
		buffer = (uint8_t*)calloc (sizeof(uint8_t),bufferLen);
		memcpy (buffer, data, len);
	}


	if (!EnigmaIOTGateway.sendDownstream (address, (uint8_t*)buffer, bufferLen, msgType, encoding)) {
		DEBUG_ERROR ("Error sending esp_now message to " MACSTR, MAC2STR (address));
	} else {
		DEBUG_DBG ("Esp-now message sent or queued correctly");
	}

	free (buffer);
}

void newNodeConnected (uint8_t * mac, uint16_t node_id) {
	char macstr[ENIGMAIOT_ADDR_LEN*3];
	mac2str (mac, macstr);
	//Serial.printf ("New node connected: %s\n", macstr);

	if (!GwOutput.newNodeSend (macstr, node_id)) {
		DEBUG_WARN ("Error sending new node %s", macstr);
	} else {
		DEBUG_DBG ("New node %s message sent", macstr);
	}
}

void nodeDisconnected (uint8_t * mac, gwInvalidateReason_t reason) {
	char macstr[ENIGMAIOT_ADDR_LEN * 3];
	mac2str (mac, macstr);
	//Serial.printf ("Node %s disconnected. Reason %u\n", macstr, reason);
	if (!GwOutput.nodeDisconnectedSend (macstr, reason)) {
		DEBUG_WARN ("Error sending node disconnected %s reason %d", macstr, reason);
	} else {
		DEBUG_DBG ("Node %s disconnected message sent. Reason %d", macstr, reason);
	}
}

//#ifdef ESP32
//void EnigmaIOTGateway_handle (void * param) {
//	for (;;) {
//		EnigmaIOTGateway.handle ();
//		vTaskDelay (0);
//	}
//}
//
//void GwOutput_handle (void* param) {
//	for (;;) {
//		GwOutput.loop ();
//		vTaskDelay (0);
//	}
//}
//
//TaskHandle_t xEnigmaIOTGateway_handle = NULL;
//TaskHandle_t gwoutput_handle = NULL;
//#endif // ESP32

void setup () {
	Serial.begin (115200); Serial.println (); Serial.println ();
#ifdef ESP8266
	ets_timer_setfn (&connectionLedTimer, flashConnectionLed, (void*)&connectionLed);
#elif defined ESP32
	
#endif
	pinMode (LED_BUILTIN, OUTPUT);
	digitalWrite (LED_BUILTIN, HIGH);
	startConnectionFlash (100);


	if (!GwOutput.loadConfig ()) {
		DEBUG_WARN ("Error reading config file");
	}

	EnigmaIOTGateway.setRxLed (BLUE_LED);
	EnigmaIOTGateway.setTxLed (RED_LED);
	EnigmaIOTGateway.onNewNode (newNodeConnected);
	EnigmaIOTGateway.onNodeDisconnected (nodeDisconnected);
	EnigmaIOTGateway.onWiFiManagerStarted (wifiManagerStarted);
	EnigmaIOTGateway.onWiFiManagerExit (wifiManagerExit);
	EnigmaIOTGateway.onDataRx (processRxData);
	EnigmaIOTGateway.begin (&Espnow_hal);

	WiFi.mode (WIFI_AP_STA);
	WiFi.begin ();

	EnigmaIOTGateway.configWiFiManager ();

	WiFi.softAP (EnigmaIOTGateway.getNetworkName (), EnigmaIOTGateway.getNetworkKey());
	stopConnectionFlash ();

	DEBUG_INFO ("STA MAC Address: %s", WiFi.macAddress ().c_str());
	DEBUG_INFO ("AP MAC Address: %s", WiFi.softAPmacAddress().c_str ());
	DEBUG_INFO ("BSSID Address: %s", WiFi.BSSIDstr().c_str ());
	   
	DEBUG_INFO ("IP address: %s", WiFi.localIP ().toString ().c_str ());
	DEBUG_INFO ("WiFi Channel: %d", WiFi.channel ());
	DEBUG_INFO ("WiFi SSID: %s", WiFi.SSID ().c_str ());
	DEBUG_INFO ("Network Name: %s", EnigmaIOTGateway.getNetworkName ());

	GwOutput.setDlCallback (onDownlinkData);
	GwOutput.begin ();

	arduinoOTAConfigure ();

#ifdef ESP32
	//xTaskCreate (EnigmaIOTGateway_handle, "handle", 10000, NULL, 1, &xEnigmaIOTGateway_handle);
	//xTaskCreatePinnedToCore (EnigmaIOTGateway_handle, "handle", 4096, NULL, 0, &xEnigmaIOTGateway_handle, 1);
	//xTaskCreatePinnedToCore (GwOutput_handle, "gwoutput", 10000, NULL, 2, &gwoutput_handle, 1);
#endif
	}

void loop () {

	GwOutput.loop ();
	EnigmaIOTGateway.handle ();
	ArduinoOTA.handle ();

}
