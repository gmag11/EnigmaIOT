/**
  * @file EnigmaIOTGatewayMQTT.ino
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief MQTT Gateway based on EnigmaIoT over ESP-NOW
  *
  * EnigmaIOT Gateway to connect nodes to MQTT broker
  */


#include <Arduino.h>

#include "GwOutput_mqtt.h"

#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif // SECURE_MQTT


#ifdef ESP32
#include <WiFi.h>
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#elif defined ESP8266
#include <ESP8266WiFi.h>
#endif // ESP32

#include <ArduinoOTA.h>

#include <CayenneLPP.h>

#include <EnigmaIOTGateway.h>
#include <helperFunctions.h>
#include <EnigmaIOTdebug.h>
#include <espnow_hal.h>
#include <ArduinoJson.h>
#include <ESPAsyncWiFiManager.h>

//#define MEAS_TEMP // Temperature measurement for Gateway monitoring using DS18B20

#ifdef MEAS_TEMP
#include <DallasTemperature.h>
#include <OneWire.h>
const time_t statusPeriod = 300 * 1000;
const int DS18B20_PIN = 16;
const int DS18B20_PREC = 12;
OneWire ow (DS18B20_PIN);
DallasTemperature ds18b20 (&ow);
DeviceAddress dsAddress;
float temperature;
#endif

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
bool connectionLedFlashing = false;

bool restartRequested = false;
time_t restartRequestTime;


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
		digitalWrite (connectionLed, LED_OFF);
	}
#endif // ESP32
}

void arduinoOTAConfigure () {
	// Port defaults to 3232
	// ArduinoOTA.setPort(3232);

	// Hostname defaults to esp3232-[MAC]
	ArduinoOTA.setHostname (EnigmaIOTGateway.getNetworkName ());

	// No authentication by default
	ArduinoOTA.setPassword (EnigmaIOTGateway.getNetworkKey (true));

	// Password can be set with it's md5 value as well
	// MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
	// ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

	ArduinoOTA.onStart ([] () {
		if (ArduinoOTA.getCommand () == U_FLASH) {
			DEBUG_WARN ("Start updating sketch");
		} else {// U_SPIFFS
			DEBUG_WARN ("Start updating filesystem");
			// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		}
						});
	ArduinoOTA.onEnd ([] () {
		DEBUG_WARN ("OTA Finished");
					  });
	ArduinoOTA.onProgress ([] (unsigned int progress, unsigned int total) {
		static bool printed = false;
		unsigned int percent = progress / (total / 100);
		digitalWrite (BLUE_LED, !digitalRead (BLUE_LED));
		// if (!(percent % 1)) {
		// 	//Serial.print ('.');
		// }
		if (!(percent % 20) && !printed && percent != 0) {
			DEBUG_WARN (" %d%%\n", percent);
			printed = true;
		} else if (percent % 20) {
			printed = false;
		}
		if (progress == total) {
			DEBUG_WARN ("OTA transfer finished");
		}
						   });
	ArduinoOTA.onError ([] (ota_error_t error) {
		DEBUG_WARN ("OTA Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) { DEBUG_WARN ("OTA Auth Failed"); } 		else if (error == OTA_BEGIN_ERROR) { DEBUG_WARN ("OTA Begin Failed"); } 		else if (error == OTA_CONNECT_ERROR) { DEBUG_WARN ("OTA Connect Failed"); } 		else if (error == OTA_RECEIVE_ERROR) { DEBUG_WARN ("OTA Receive Failed"); } 		else if (error == OTA_END_ERROR) { DEBUG_WARN ("OTA End Failed"); }
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
		if (data[0] == VERSION_ANS && length >= 4) {
			DEBUG_INFO ("Version message: %d.%d.%d", data[1], data[2], data[3]);
			Node* node = EnigmaIOTGateway.getNodes ()->getNodeFromName (macStr);
			if (node) {
				node->setVersion (data[1], data[2], data[3]);
			}
		}
		GwOutput.outputControlSend (macStr, data, length);
	}
}

void doRestart () {
	DEBUG_WARN ("Restart requested");
	const size_t capacity = JSON_OBJECT_SIZE (1);
	size_t len;
	char* payload;

	DynamicJsonDocument doc (capacity);

	doc["action"] = "restart";

	len = measureJson (doc) + 1;
	payload = (char*)malloc (len);
	serializeJson (doc, (char*)payload, len);
	char addr[] = "gateway";
	GwOutput.outputDataSend (addr, payload, len - 1);
	free (payload);

	restartRequested = true;
	restartRequestTime = millis ();

}

#if SUPPORT_HA_DISCOVERY
void processHADiscovery (const char* topic, char* message, size_t len) {
    DEBUG_INFO ("About to process HA discovery. Len: %d - %s --> %.*s", len, topic, len, message);
    GwOutput.rawMsgSend (topic, message, len, true);
}
#endif

void processRxData (uint8_t* mac, uint8_t* buffer, uint8_t length, uint16_t lostMessages, bool control, gatewayPayloadEncoding_t payload_type, char* nodeName = NULL) {
	uint8_t* addr = mac;
	size_t pld_size = 0;
	const int PAYLOAD_SIZE = 1024; // Max MQTT payload in PubSubClient library normal operation.

	char payload[PAYLOAD_SIZE];

	char mac_str[ENIGMAIOT_ADDR_LEN * 3];
	mac2str (addr, mac_str);

	if (control) {
		processRxControlData (nodeName ? nodeName : mac_str, buffer, length);
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

	GwOutput.outputDataSend (nodeName ? nodeName : mac_str, payload, pld_size);
	DEBUG_INFO ("Published data message from %s, length %d: %s, Encoding 0x%02X", nodeName ? nodeName : mac_str, pld_size, payload, payload_type);
	if (lostMessages > 0) {
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"lostMessages\":%u}", lostMessages);
		GwOutput.outputDataSend (nodeName ? nodeName : mac_str, payload, pld_size, GwOutput_data_type::lostmessages);
		DEBUG_INFO ("Published MQTT from %s: %s", nodeName ? nodeName : mac_str, payload);
	}
#if ENABLE_STATUS_MESSAGES
    pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"rssi\":%d,\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%.2f}",
                         EnigmaIOTGateway.getNodes()->getNodeFromMAC((uint8_t*)mac)->getRSSI(),
                         EnigmaIOTGateway.getPER ((uint8_t*)mac),
						 EnigmaIOTGateway.getErrorPackets ((uint8_t*)mac),
						 EnigmaIOTGateway.getTotalPackets ((uint8_t*)mac),
						 EnigmaIOTGateway.getPacketsHour ((uint8_t*)mac));
	GwOutput.outputDataSend (nodeName ? nodeName : mac_str, payload, pld_size, GwOutput_data_type::status);
	DEBUG_INFO ("Published MQTT from %s: %s", nodeName ? nodeName : mac_str, payload);
#endif
}

void onDownlinkData (uint8_t* address, char* nodeName, control_message_type_t msgType, char* data, unsigned int len) {
	uint8_t* buffer;
	unsigned int bufferLen = len;
	gatewayPayloadEncoding_t encoding = ENIGMAIOT;

	if (nodeName) {
		DEBUG_INFO ("DL Message for %s. Type 0x%02X", nodeName, msgType);
	} else {
		DEBUG_INFO ("DL Message for " MACSTR ". Type 0x%02X", MAC2STR (address), msgType);
	}
	DEBUG_DBG ("Data: %.*s Length: %d", len, data, len);

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
		buffer = (uint8_t*)calloc (sizeof (uint8_t), bufferLen);
		memcpy (buffer, data, len);
	}


	if (!EnigmaIOTGateway.sendDownstream (address, buffer, bufferLen, msgType, encoding, nodeName)) {
		if (nodeName) {
			DEBUG_WARN ("Error sending esp_now message to %s", nodeName);
		} else {
			DEBUG_WARN ("Error sending esp_now message to " MACSTR, MAC2STR (address));
		}
	} else {
		DEBUG_DBG ("Esp-now message sent or queued correctly");
	}

	free (buffer);
}

void newNodeConnected (uint8_t* mac, uint16_t node_id, char* nodeName = nullptr) {

	//Serial.printf ("New node connected: %s\n", macstr);

	if (nodeName) {
		if (!GwOutput.newNodeSend (nodeName, node_id)) {
			DEBUG_WARN ("Error sending new node %s", nodeName);
		} else {
			DEBUG_DBG ("New node %s message sent", nodeName);
		}
	} else {
		char macstr[ENIGMAIOT_ADDR_LEN * 3];
		mac2str (mac, macstr);
		if (!GwOutput.newNodeSend (macstr, node_id)) {
			DEBUG_WARN ("Error sending new node %s", macstr);
		} else {
			DEBUG_DBG ("New node %s message sent", macstr);
		}
	}

}

void nodeDisconnected (uint8_t* mac, gwInvalidateReason_t reason) {
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
	Serial.begin (921600); Serial.println (); Serial.println ();

#ifdef ESP32
// Turn-off the 'brownout detector' to avoid random restarts during wake up,
// normally due to bad quality regulator on board
	WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0);
#endif

#ifdef ESP8266
	ets_timer_setfn (&connectionLedTimer, flashConnectionLed, (void*)&connectionLed);
#elif defined ESP32

#endif
	pinMode (LED_BUILTIN, OUTPUT);
	digitalWrite (LED_BUILTIN, LED_ON);

#ifdef MEAS_TEMP
    ds18b20.begin ();
    
    DEBUG_WARN ("Found %u sensors", ds18b20.getDeviceCount ());
    
    if (ds18b20.getAddress (dsAddress, 0)) {
		DEBUG_WARN ("DS18B20 address: %02X %02X %02X %02X %02X %02X %02X %02X",
					dsAddress[0], dsAddress[1], dsAddress[2], dsAddress[3],
					dsAddress[4], dsAddress[5], dsAddress[6], dsAddress[7]);
	} else {
		DEBUG_WARN ("No DS18B20 found");
	}
	ds18b20.setWaitForConversion (false);
	ds18b20.setResolution (DS18B20_PREC);
#endif // MEAS_TEMP

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
#if SUPPORT_HA_DISCOVERY
    EnigmaIOTGateway.onHADiscovery (processHADiscovery);
#endif
	EnigmaIOTGateway.onGatewayRestartRequested (doRestart);

	EnigmaIOTGateway.begin (&Espnow_hal);

	WiFi.mode (WIFI_AP_STA);
	WiFi.begin ();

	EnigmaIOTGateway.configWiFiManager ();

	WiFi.softAP (EnigmaIOTGateway.getNetworkName (), EnigmaIOTGateway.getNetworkKey (true));
	stopConnectionFlash ();

	DEBUG_INFO ("STA MAC Address: %s", WiFi.macAddress ().c_str ());
	DEBUG_INFO ("AP MAC Address: %s", WiFi.softAPmacAddress ().c_str ());
	DEBUG_INFO ("BSSID Address: %s", WiFi.BSSIDstr ().c_str ());

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

#ifdef MEAS_TEMP
void sendStatus (float temperature) {
	const size_t capacity = JSON_OBJECT_SIZE (1) + JSON_OBJECT_SIZE (3) + 30;;
	size_t len;
	char* payload;

	DynamicJsonDocument doc (capacity);

    JsonObject status = doc.createNestedObject ("status");
    if (temperature > -100) {
        status["temp"] = temperature;
    }
    status["nodes"] = EnigmaIOTGateway.getActiveNodesNumber ();
	status["mem"] = ESP.getFreeHeap ();

	len = measureJson (doc) + 1;
	payload = (char*)malloc (len);
	serializeJson (doc, (char*)payload, len);
	char addr[] = "gateway";
	GwOutput.outputDataSend (addr, payload, len - 1);
	free (payload);
}
#endif // MEAS_TEMP

void loop () {
	GwOutput.loop ();
	EnigmaIOTGateway.handle ();
	ArduinoOTA.handle ();

#ifdef MEAS_TEMP
	static bool tempRequested = false;
	static time_t lastTempTime = 0;

	if (ds18b20.validAddress (dsAddress)) {
        if ((millis () - lastTempTime > statusPeriod && !tempRequested) || !lastTempTime) {
            if (ds18b20.requestTemperaturesByIndex (0)) {
                DEBUG_INFO ("Temperature requested");
                lastTempTime = millis ();
                tempRequested = true;
            } else {
                DEBUG_WARN ("Temperature request error");
            }
		}
		if (tempRequested) {
			if (ds18b20.isConversionComplete ()) {
				temperature = ds18b20.getTempC (dsAddress);
				sendStatus (temperature);
				DEBUG_WARN ("Temperature: %f", temperature);
				tempRequested = false;
			}
		}
	}
#endif // MEAS_TEMP

	if (restartRequested) {
		if (millis () - restartRequestTime > 100) {
			ESP.restart ();
		}
	}
}
