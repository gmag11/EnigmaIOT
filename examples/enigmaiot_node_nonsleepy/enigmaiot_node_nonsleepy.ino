/**
  * @file enigmaiot_node_nonsleepy.ino
  * @version 0.8.1
  * @date 17/01/2020
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW, in non sleeping mode
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

#ifndef ESP8266
#error Node only supports ESP8266 platform
#endif

#define BLUE_LED LED_BUILTIN
#define OLR_BUTTON 14

bool button_pushed = false;
bool intr_set = true;

ADC_MODE (ADC_VCC);

void connectEventHandler () {
	Serial.println ("Connected");
}

void disconnectEventHandler () {
	Serial.println ("Disconnected");
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, nodeMessageType_t command) {
	char macstr[18];
	String commandStr;

	mac2str (mac, macstr);
	Serial.println ();
	Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));
	if (command == nodeMessageType_t::DOWNSTREAM_DATA_GET)
		commandStr = "GET";
	else if (command == nodeMessageType_t::DOWNSTREAM_DATA_SET)
		commandStr = "SET";
	else
		return;

	Serial.printf ("Command %s\n", commandStr.c_str ());
	Serial.printf ("Data: %s\n", printHexBuffer (buffer, length));

}

IRAM_ATTR void push_button () {
	detachInterrupt (OLR_BUTTON);
	button_pushed = true;
	intr_set = false;
}

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();
	pinMode (OLR_BUTTON, INPUT_PULLUP);
	attachInterrupt (OLR_BUTTON, push_button,FALLING);

	EnigmaIOTNode.setLed (BLUE_LED);
	//pinMode (BLUE_LED, OUTPUT);
	//digitalWrite (BLUE_LED, HIGH); // Turn on LED
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);

	EnigmaIOTNode.begin (&Espnow_hal, NULL, NULL, true, false);
}

void loop () {
	static int last_button;
	static int diff_button;

	EnigmaIOTNode.handle ();

	CayenneLPP msg (20);
	//if (button_pushed) {
	//	diff_button = millis () - last_button;
	//	last_button = millis ();
	//	Serial.printf ("%d button\n", diff_button);
	//	msg.addUnixTime (0, last_button);
	//	if (!EnigmaIOTNode.sendData (msg.getBuffer (), msg.getSize ())) {
	//		Serial.println ("---- Error sending data");
	//	} else {
	//		Serial.println ("---- Data sent");
	//	}
	//	button_pushed = false;
	//	//attachInterrupt (OLR_BUTTON, push_button, FALLING);
	//}

	//if (!intr_set) {
	//	if (millis () - last_button > 80) {
	//		attachInterrupt (OLR_BUTTON, push_button, FALLING);
	//	}
	//}

	static time_t lastSensorData;
	static const time_t SENSOR_PERIOD = 30;
	if (millis () - lastSensorData > SENSOR_PERIOD) {
		lastSensorData = millis ();
		
		// Read sensor data
		msg.addUnixTime (0, lastSensorData);
		//msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
		//Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
		//msg.addTemperature (1, 20.34);
		// Read sensor data
		
		Serial.printf ("Trying to send: %s\n", printHexBuffer (msg.getBuffer (), msg.getSize ()));

		if (!EnigmaIOTNode.sendUnencryptedData (msg.getBuffer (), msg.getSize ())) {
			Serial.println ("---- Error sending data");
		} else {
			//Serial.println ("---- Data sent");
		}

	}
}
