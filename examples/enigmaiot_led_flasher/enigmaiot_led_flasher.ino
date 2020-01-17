/**
  * @file enigmaiot_led_flasher.ino
  * @version 0.8.1
  * @date 17/01/2020
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW using non sleeping mode. Demonstrate clock synchronisation function
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
//#include <CRC32.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <DNSServer.h>

#define BLUE_LED LED_BUILTIN

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

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();
	
	//EnigmaIOTNode.setLed (BLUE_LED);
	pinMode (BLUE_LED, OUTPUT);
	digitalWrite (BLUE_LED, HIGH); // Turn on LED
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);
	EnigmaIOTNode.enableClockSync ();

	EnigmaIOTNode.begin (&Espnow_hal, NULL, NULL, true, false);
}

void loop () {

	EnigmaIOTNode.handle ();

	static const time_t PERIOD = 3000;
	static const time_t FLASH_DURATION = 100;
	static time_t clock;

	clock = EnigmaIOTNode.clock () % PERIOD;

	if (EnigmaIOTNode.hasClockSync () && EnigmaIOTNode.isRegistered()) {
		if (clock >= 0 && clock < FLASH_DURATION) {
			digitalWrite (BLUE_LED, LOW); // Turn on LED
		} else {
			digitalWrite (BLUE_LED, HIGH); // Turn on LED
		}
	}
	
	CayenneLPP msg (MAX_DATA_PAYLOAD_SIZE);

	static time_t lastSensorData;
	static const time_t SENSOR_PERIOD = 10000;
	if (millis () - lastSensorData > SENSOR_PERIOD) {
		lastSensorData = millis ();
		
		// Read sensor data
		msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
		Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
		msg.addTemperature (1, 20.34);
		// Read sensor data
		
		Serial.printf ("Trying to send: %s\n", printHexBuffer (msg.getBuffer (), msg.getSize ()));

		if (!EnigmaIOTNode.sendData (msg.getBuffer (), msg.getSize ())) {
			Serial.println ("---- Error sending data");
		} else {
			Serial.println ("---- Data sent");
		}

	}
}
