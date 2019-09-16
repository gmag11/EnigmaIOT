/**
  * @file enigmaiot_gateway.ino
  * @version 0.4.0
  * @date 10/09/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  *
  * Communicates with a serial to MQTT gateway to send data to any IoT platform
  */

#include <Arduino.h>
#include <CayenneLPP.h>
#include <EnigmaIOTGateway.h>
#include <espnow_hal.h>

#define BLUE_LED 2
#define RED_LED 2

// DOWNLINK MESSAGES
#define GET_VERSION "get/version"
#define GET_VERSION_ANS "result/version"
#define GET_SLEEP "get/sleeptime"
#define GET_SLEEP_ANS "result/sleeptime"
#define SET_SLEEP "set/sleeptime"
#define SET_OTA "set/ota"
#define SET_OTA_ANS "result/ota"
#define SET_IDENTIFY "set/identify"

void processRxControlData (char* macStr, const uint8_t* data, uint8_t length) {
	switch (data[0]) {
		case control_message_type::VERSION_ANS:
			Serial.printf ("~/%s/%s;", macStr, GET_VERSION_ANS);
			Serial.write (data + 1, length - 1);
			Serial.println ();
			break;
		case control_message_type::SLEEP_ANS:
			uint32_t sleepTime;
			memcpy (&sleepTime, data + 1, sizeof (sleepTime));
			Serial.printf ("~/%s/%s;%d\n", macStr, GET_SLEEP_ANS, sleepTime);
			break;
		case control_message_type::OTA_ANS:
			Serial.printf ("~/%s/%s;", macStr, SET_OTA_ANS);
			switch (data[1]) {
				case ota_status::OTA_STARTED:
					Serial.printf ("OTA Started\n");
					break;
				case ota_status::OTA_START_ERROR:
					Serial.printf ("OTA Start error\n");
					break;
				case ota_status::OTA_OUT_OF_SEQUENCE:
					Serial.printf ("OTA out of sequence error\n");
					break;
				case ota_status::OTA_CHECK_OK:
					Serial.printf ("OTA check OK\n");
					break;
				case ota_status::OTA_CHECK_FAIL:
					Serial.printf ("OTA check failed\n");
					break;
				case ota_status::OTA_TIMEOUT:
					Serial.printf ("OTA timeout\n");
					break;
				case ota_status::OTA_FINISHED:
					Serial.printf ("OTA finished OK\n");
					break;
				default:
					Serial.println ();
			}
			break;
	}
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages, bool control) {
	const int capacity = JSON_OBJECT_SIZE(4) * 14;
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
	}
	if (data.indexOf (GET_SLEEP) != -1) {
		return control_message_type::SLEEP_GET;
	}
	if (data.indexOf (SET_SLEEP) != -1) {
		return control_message_type::SLEEP_SET;
	}
	if (data.indexOf (SET_OTA) != -1) {
		return control_message_type::OTA;
	}
	if (data.indexOf (SET_IDENTIFY) != -1) {
		DEBUG_WARN ("IDENTIFY MESSAGE %s", data.c_str ());
		return control_message_type::IDENTIFY;
	}
	return control_message_type::USERDATA;
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
	if (msgType != control_message_type::USERDATA) {
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
	Serial.begin (115200); Serial.println (); Serial.println ();

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
