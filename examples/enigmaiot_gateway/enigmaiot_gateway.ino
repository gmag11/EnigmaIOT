/**
  * @file enigmaiot_gateway.ino
  * @version 0.7.0
  * @date 31/12/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  *
  * *DEPRECATED*
  *
  * Communicates with a serial to MQTT gateway to send data to any IoT platform
  */

#include <Arduino.h>
#include <CayenneLPP.h>
#include <EnigmaIOTGateway.h>
#include <espnow_hal.h>

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Curve25519.h>
#include <CRC32.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <ESPAsyncWiFiManager.h>
#include <DNSServer.h>

#define BLUE_LED 2
#define RED_LED 2

// DOWNLINK MESSAGES
#define GET_VERSION      "get/version"
#define GET_VERSION_ANS  "result/version"
#define GET_SLEEP        "get/sleeptime"
#define GET_SLEEP_ANS    "result/sleeptime"
#define SET_SLEEP        "set/sleeptime"
#define SET_OTA          "set/ota"
#define SET_OTA_ANS      "result/ota"
#define SET_IDENTIFY     "set/identify"
#define SET_RESET_CONFIG "set/reset"
#define SET_RESET_ANS    "result/reset"
#define GET_RSSI         "get/rssi"
#define GET_RSSI_ANS     "result/rssi"
#define SET_USER_DATA    "set/data"
#define GET_USER_DATA    "get/data"

void processRxControlData (char* macStr, const uint8_t* data, uint8_t length) {
	switch (data[0]) {
		case control_message_type::VERSION_ANS:
			Serial.printf ("~/%s/%s;{\"version\":\"", macStr, GET_VERSION_ANS);
			Serial.write (data + 1, length - 1);
			Serial.println ("\"}");
			break;
		case control_message_type::SLEEP_ANS:
			uint32_t sleepTime;
			memcpy (&sleepTime, data + 1, sizeof (sleepTime));
			Serial.printf ("~/%s/%s;{\"sleeptime\":%d}\n", macStr, GET_SLEEP_ANS, sleepTime);
			break;
		case control_message_type::RESET_ANS:
			Serial.printf ("~/%s/%s;{}\n", macStr, SET_RESET_ANS);
			break;
		case control_message_type::RSSI_ANS:
			Serial.printf ("~/%s/%s;{\"rssi\":%d,\"channel\":%u}\n", macStr, GET_RSSI_ANS, (int8_t)data[1], data[2]);
			break;
		case control_message_type::OTA_ANS:
			Serial.printf ("~/%s/%s;", macStr, SET_OTA_ANS);
			switch (data[1]) {
				case ota_status::OTA_STARTED:
					Serial.printf ("{\"result\":\"OTA Started\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_START_ERROR:
					Serial.printf ("{\"result\":\"OTA Start error\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_OUT_OF_SEQUENCE:
					uint16_t lastGoodIdx;
					memcpy ((uint8_t*)& lastGoodIdx, data + 2, sizeof (uint16_t));
					Serial.printf ("{\"last_chunk\":%d,\"result\":\"OTA out of sequence error\",\"status\":%u}\n", lastGoodIdx, data[1]);
					break;
				case ota_status::OTA_CHECK_OK:
					Serial.printf ("{\"result\":\"OTA check OK\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_CHECK_FAIL:
					Serial.printf ("{\"result\":\"OTA check failed\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_TIMEOUT:
					Serial.printf ("{\"result\":\"OTA timeout\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_FINISHED:
					Serial.printf ("{\"result\":\"OTA finished OK\",\"status\":%u}\n", data[1]);
					break;
				default:
					Serial.println ();
			}
			break;
	}
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages, bool control) {
	const int capacity = JSON_ARRAY_SIZE (25) + 25 * JSON_OBJECT_SIZE (4);
	StaticJsonDocument<capacity> jsonBuffer;
	JsonArray root = jsonBuffer.createNestedArray ();
	CayenneLPP cayennelpp (MAX_DATA_PAYLOAD_SIZE);

	char macstr[18];
	mac2str (mac, macstr);

	if (control) {
		processRxControlData (macstr, buffer, length);
		return;
	}

	//Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));

	cayennelpp.decode ((uint8_t *)buffer, length, root);
	//root.prettyPrintTo (Serial);
	//Serial.println ();
	Serial.printf ("~/%s/data;", macstr);
	serializeJson (root, Serial);
	//root.printTo (Serial);

	Serial.println ();
	if (lostMessages > 0) {
		//serial.printf ("%u lost messages\n", lostmessages);
		Serial.printf ("~/%s/debug/lostmessages;%u\n", macstr, lostMessages);
	}
	Serial.printf ("~/%s/status;{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%.2f}\n",
		macstr,
		EnigmaIOTGateway.getPER ((uint8_t*)mac),
		EnigmaIOTGateway.getErrorPackets ((uint8_t*)mac),
		EnigmaIOTGateway.getTotalPackets ((uint8_t*)mac),
		EnigmaIOTGateway.getPacketsHour ((uint8_t*)mac));
	//Serial.println ();
}

control_message_type_t checkMsgType (String data) {
	if (data.indexOf (GET_VERSION) != -1) {
		return control_message_type::VERSION;
	} else
	if (data.indexOf (GET_SLEEP) != -1) {
		return control_message_type::SLEEP_GET;
	} else
	if (data.indexOf (SET_SLEEP) != -1) {
		return control_message_type::SLEEP_SET;
	} else
	if (data.indexOf (SET_OTA) != -1) {
		return control_message_type::OTA;
	} else
	if (data.indexOf (SET_IDENTIFY) != -1) {
		DEBUG_WARN ("IDENTIFY MESSAGE %s", data.c_str ());
		return control_message_type::IDENTIFY;
	} else
	if (data.indexOf (SET_RESET_CONFIG) != -1) {
		DEBUG_WARN ("RESET CONFIG MESSAGE %s", data.c_str ());
		return control_message_type::RESET;
	} else
	if (data.indexOf (GET_RSSI) != -1) {
		DEBUG_INFO ("GET RSSI MESSAGE %s", data.c_str ());
		return control_message_type::RSSI_GET;
	} else 
	if (data == SET_USER_DATA) {
		DEBUG_INFO ("USER DATA %s", data.c_str ());
		return control_message_type::USERDATA_SET;
	} else 
	if (data == GET_USER_DATA) {
		DEBUG_INFO ("USER DATA %s", data.c_str ());
		return control_message_type::USERDATA_GET;
	} else
		return control_message_type::INVALID;
}

void onSerial (String message) {
	uint8_t addr[6];

	DEBUG_VERBOSE ("Downlink message: %s", message.c_str ());
	String addressStr = message.substring (message.indexOf ('/') + 1, message.indexOf ('/', 2));
	DEBUG_INFO ("Downlink message to: %s", addressStr.c_str ());
	if (!str2mac (addressStr.c_str (), addr)) {
		DEBUG_ERROR ("Not a mac address");
		return;
	}
	String dataStr = message.substring (message.indexOf ('/', 2) + 1);
	dataStr.trim ();

	control_message_type_t msgType = checkMsgType (dataStr);

	// Add end of string to all control messages
	if (msgType != control_message_type::USERDATA_GET && msgType != control_message_type::USERDATA_SET) {
		dataStr = dataStr.substring (dataStr.indexOf (';') + 1);
		dataStr += '\0';
	}

	DEBUG_VERBOSE ("Message %s", dataStr.c_str ());
	DEBUG_INFO ("Message type %d", msgType);
	DEBUG_INFO ("Data length %d", dataStr.length ());
	if (!EnigmaIOTGateway.sendDownstream (addr, (uint8_t*)dataStr.c_str (), dataStr.length (), msgType)) {
		DEBUG_ERROR ("Error sending esp_now message to %s", addressStr.c_str ());
	}
	else {
		DEBUG_DBG ("Esp-now message sent or queued correctly");
	}
}

void newNodeConnected (uint8_t * mac) {
	char macstr[18];
	mac2str (mac, macstr);
	//Serial.printf ("New node connected: %s\n", macstr);
	Serial.printf ("~/%s/hello\n", macstr);
}

void nodeDisconnected (uint8_t * mac, gwInvalidateReason_t reason) {
	char macstr[18];
	mac2str (mac, macstr);
	//Serial.printf ("Node %s disconnected. Reason %u\n", macstr, reason);
	Serial.printf ("~/%s/bye;{\"reason\":%u}\n", macstr, reason);

}

void setup () {
	Serial.begin (500000); Serial.println (); Serial.println ();

	EnigmaIOTGateway.setRxLed (BLUE_LED);
	EnigmaIOTGateway.setTxLed (RED_LED);
	EnigmaIOTGateway.onNewNode (newNodeConnected);
	EnigmaIOTGateway.onNodeDisconnected (nodeDisconnected);
	EnigmaIOTGateway.begin (&Espnow_hal);
	EnigmaIOTGateway.onDataRx (processRxData);
}

void loop () {
	String message;

	EnigmaIOTGateway.handle ();

	while (Serial.available () != 0) {
		message = Serial.readStringUntil ('\n');
		message.trim ();
		if (message[0] == '%') {
			onSerial (message);
		}
	}
}
