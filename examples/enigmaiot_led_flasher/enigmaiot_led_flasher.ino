/**
  * @file enigmaiot_led_flasher.ino
  * @version 0.9.4
  * @date 31/07/2020
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW using non sleeping mode. Demonstrate clock synchronisation function
  *
  * Sensor reading code is mocked on this example. You can implement any other code you need for your specific need
  */

#include <Arduino.h>
#include <EnigmaIOTNode.h>
#include <espnow_hal.h>
#include <CayenneLPP.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESPAsyncTCP.h> // Comment to compile for ESP32
#include <Hash.h>
#elif defined ESP32
#include <WiFi.h>
//#include <AsyncTCP.h> // Comment to compile for ESP8266
#include <SPIFFS.h>
#include <Update.h>
#include <driver/adc.h>
#include "esp_wifi.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#endif
#include <ArduinoJson.h>
#include <Curve25519.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <DNSServer.h>
#include <FS.h>

#define LED_ON LOW
#define LED_OFF !LED_ON

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // ESP32 boards normally have a LED in GPIO2 or GPIO5
#endif // !LED_BUILTIN

#define BLUE_LED LED_BUILTIN
constexpr auto RESET_PIN = -1;

#ifdef ESP8266
ADC_MODE (ADC_VCC);
#endif

void connectEventHandler () {
	Serial.println ("Connected");
}

void disconnectEventHandler (nodeInvalidateReason_t reason) {
	Serial.printf ("Unregistered. Reason: %d\n", reason);
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) {
	char macstr[ENIGMAIOT_ADDR_LEN * 3];
	String commandStr;
	uint8_t tempBuffer[MAX_MESSAGE_LENGTH];

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
	Serial.printf ("Encoding: 0x%02X\n", payloadEncoding);

	CayenneLPP lpp (MAX_DATA_PAYLOAD_SIZE);
	DynamicJsonDocument doc (1000);
	JsonArray root;
	memcpy (tempBuffer, buffer, length);

	switch (payloadEncoding) {
	case CAYENNELPP:
		root = doc.createNestedArray ();
		lpp.decode (tempBuffer, length, root);
		serializeJsonPretty (doc, Serial);
		break;
	case MSG_PACK:
		deserializeMsgPack (doc, tempBuffer, length);
		serializeJsonPretty (doc, Serial);
		break;
	default:
		DEBUG_WARN ("Non supported encoding; %d", payloadEncoding);
	}
}

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();

#ifdef ESP32
	// Turn-off the 'brownout detector' to avoid random restarts during wake up,
	// normally due to bad quality regulator on board
	WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0);
#endif

	//EnigmaIOTNode.setLed (BLUE_LED);
	pinMode (BLUE_LED, OUTPUT);
	digitalWrite (BLUE_LED, LED_OFF); // Turn off LED
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);
	EnigmaIOTNode.enableClockSync ();

	EnigmaIOTNode.begin (&Espnow_hal, NULL, NULL, true, false);

	uint8_t macAddress[ENIGMAIOT_ADDR_LEN];
#ifdef ESP8266
	if (wifi_get_macaddr (STATION_IF, macAddress))
#elif defined ESP32
	if ((esp_wifi_get_mac (WIFI_IF_STA, macAddress) == ESP_OK))
#endif
	{
		EnigmaIOTNode.setNodeAddress (macAddress);
		char macStr[ENIGMAIOT_ADDR_LEN * 3];
		DEBUG_DBG ("Node address set to %s", mac2str (macAddress, macStr));
	} else {
		DEBUG_WARN ("Node address error");
	}

}

void loop () {

	EnigmaIOTNode.handle ();

	static const time_t PERIOD = 3000;
	static const time_t FLASH_DURATION = 100;
	static time_t clock;

	clock = EnigmaIOTNode.clock () % PERIOD;

	if (EnigmaIOTNode.hasClockSync () && EnigmaIOTNode.isRegistered ()) {
		if (clock >= 0 && clock < FLASH_DURATION) {
			digitalWrite (BLUE_LED, LED_ON); // Turn on LED
		} else {
			digitalWrite (BLUE_LED, LED_OFF); // Turn on LED
		}
	}

	CayenneLPP msg (MAX_DATA_PAYLOAD_SIZE);

	static time_t lastSensorData;
	static const time_t SENSOR_PERIOD = 10000;
	if (millis () - lastSensorData > SENSOR_PERIOD) {
		lastSensorData = millis ();

		// Read sensor data
#ifdef ESP8266
		msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
		Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
#elif defined ESP32
		msg.addAnalogInput (0, (float)(analogRead (ADC1_CHANNEL_0_GPIO_NUM) * 3.6 / 4096));
		Serial.printf ("Vcc: %f\n", (float)(analogRead (ADC1_CHANNEL_0_GPIO_NUM) * 3.6 / 4096));
#endif
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
