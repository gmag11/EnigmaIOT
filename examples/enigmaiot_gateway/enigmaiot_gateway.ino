/**
  * @file enigmaiot_gateway.ino
  * @version 0.2.0
  * @date 28/06/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  *
  * Communicates with a serial to MQTT gateway to send data to any IoT platform
  */

#include <Arduino.h>
#include <CayenneLPPDec.h>
#include <EnigmaIOTGateway.h>
#include <espnow_hal.h>

#define BLUE_LED 2
#define RED_LED 16

// DOWNLINK MESSAGES
#define GET_VERSION "get/version"
#define GET_VERSION_ANS "result/version"
#define GET_SLEEP "get/sleeptime"
#define GET_SLEEP_ANS "result/sleeptime"
#define SET_SLEEP "set/sleeptime"
#define SET_OTA "set/ota"
#define OTA_ANS "result/ota"


void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages) {
	StaticJsonDocument<256> jsonBuffer;
	JsonArray root = jsonBuffer.createNestedArray ();

	char macstr[18];
	mac2str (mac, macstr);
	//Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));

	CayenneLPPDec::ParseLPP (buffer, length, root);
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
	Serial.printf ("~/%s/status;{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%f}\n",
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
	return control_message_type::USERDATA;
}

void onSerial (String message) {
	uint8_t addr[6];

	DEBUG_VERBOSE ("Downlink message: %s", message.c_str ());
	String addressStr = message.substring (message.indexOf ('/') + 1, message.indexOf ('/', 2));
	DEBUG_INFO ("Downlink message from: %s", addressStr.c_str ());
	if (!str2mac (addressStr.c_str (), addr)) {
		DEBUG_ERROR ("Not a mac address");
		return;
	}
	String dataStr = message.substring (message.indexOf ('/', 2) + 1);
	dataStr.trim ();

	control_message_type_t msgType = checkMsgType (dataStr);
	DEBUG_INFO ("Message type %d", msgType);
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

	initWiFi ();
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
