/**
  * @file enigmaiot_node.ino
  * @version 0.9.0
  * @date 20/05/2020
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW that uses MessagePack as payload encoding
  *
  * Sensor reading code is mocked on this example. You can implement any other code you need for your specific need
  */

#ifndef ESP8266
#error Node only supports ESP8266 platform
#endif

#include <Arduino.h>
#include <EnigmaIOTNode.h>
#include <espnow_hal.h>
#include <CayenneLPP.h>

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Curve25519.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <DNSServer.h>

#define BLUE_LED LED_BUILTIN
constexpr auto RESET_PIN = 13;

//ADC_MODE (ADC_VCC);

void connectEventHandler () {
	Serial.println ("Registered");
}

void disconnectEventHandler (nodeInvalidateReason_t reason) {
	Serial.printf ("Unregistered. Reason: %d\n", reason);
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t encoding) {
	char macstr[18];
	String commandStr;
	//void* data;
	uint8_t tempBuffer[MAX_MESSAGE_LENGTH];

	mac2str (mac, macstr);
	Serial.println ();
	Serial.printf ("Data from %s\n", macstr);
	if (command == nodeMessageType_t::DOWNSTREAM_DATA_GET)
		commandStr = "GET";
	else if (command == nodeMessageType_t::DOWNSTREAM_DATA_SET)
		commandStr = "SET";
	else
		return;

	Serial.printf ("Command %s\n", commandStr.c_str ());
	Serial.printf ("Data: %s\n", printHexBuffer (buffer, length));
	Serial.printf ("Encoding: 0x%02X\n", encoding);

	//for (int i = 0; i < length; i++) {
	//	Serial.print ((char)buffer[i]);
	//}
	//Serial.println ();
	//Serial.println ();
	CayenneLPP lpp(MAX_DATA_PAYLOAD_SIZE);
	DynamicJsonDocument doc (1000);
	JsonArray root;
	memcpy (tempBuffer, buffer, length);

	switch (encoding) {
	case CAYENNELPP:
		root = doc.createNestedArray ();
		lpp.decode (tempBuffer, length, root);
		serializeJsonPretty (doc, Serial);
		break;
	case MSG_PACK:
		deserializeMsgPack (doc, tempBuffer, length);
		serializeJsonPretty (doc, Serial);
		break;
	}
}

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();
	time_t start = millis ();
	
	EnigmaIOTNode.setLed (BLUE_LED);
	EnigmaIOTNode.setResetPin (RESET_PIN);
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);

	EnigmaIOTNode.begin (&Espnow_hal);
	
	// Put here your code to read sensor and compose buffer
	const size_t capacity = JSON_OBJECT_SIZE (5);
	DynamicJsonDocument json (capacity);

	json["V"] = (float)(ESP.getVcc ()) / 1000;
	json["tem"] = 203;
	json["din"] = 123;
	json["pres"] = 1007;
	json["curr"] = 2.43;

	int len = measureMsgPack (json) + 1;
	uint8_t* buffer = (uint8_t*)malloc (len);
	len = serializeMsgPack (json, (char *)buffer, len);

	Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
	Serial.printf ("Message Len %d\n", len);
	// End of user code

	Serial.printf ("Trying to send: %s\n", printHexBuffer (buffer, len));

    // Send buffer data
	if (!EnigmaIOTNode.sendData (buffer, len, MSG_PACK)) {
		Serial.println ("---- Error sending data");
	} else {
		Serial.println ("---- Data sent");
	}
	Serial.printf ("Total time: %d ms\n", millis() - start);

	free (buffer);

    // Go to sleep
	EnigmaIOTNode.sleep ();
}

void loop () {

	EnigmaIOTNode.handle ();

}
