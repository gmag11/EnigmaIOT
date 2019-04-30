/**
  * @file enigmaiot_gateway.ino
  * @version 0.1.0
  * @date 09/03/2019
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

void onSerial (String message) {
	uint8_t addr[6];

	DEBUG_INFO ("Downlink message: %s", message.c_str ());
	String addressStr = message.substring (message.indexOf ('/') + 1, message.indexOf ('/', 2));
	str2mac (addressStr.c_str (), addr);
	String dataStr = message.substring (message.indexOf ('/', 2) + 1);
	const void* data = dataStr.c_str ();
	DEBUG_INFO ("Address: %s", addressStr.c_str ());
	EnigmaIOTGateway.sendDownstream (addr, (uint8_t*)data, dataStr.length ());
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
	EnigmaIOTGateway.begin (&Espnow_hal, (uint8_t*)NETWORK_KEY);
	EnigmaIOTGateway.onDataRx (processRxData);
}

void loop () {
	String message;

	EnigmaIOTGateway.handle ();

	while (Serial.available () != 0) {
		message = Serial.readStringUntil ('\n');
		if (message[0] == '*') {
			onSerial (message);
		}
	}
}
