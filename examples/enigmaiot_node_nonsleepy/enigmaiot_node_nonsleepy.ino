/**
  * @file enigmaiot_node_nonsleepy.ino
  * @version 0.9.4
  * @date 31/07/2020
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW, in non sleeping mode
  *
  * Sensor reading code is mocked on this example. You can implement any other code you need for your specific need
  */

#if !defined ESP8266 && !defined ESP32
#error Node only supports ESP8266 or ESP32 platform
#endif

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

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // ESP32 boards normally have a LED in GPIO3 or GPIO5
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
	bool broadcast=false;
	uint8_t _command = command;

	if (_command & 0x80)
		broadcast = true;

	_command = (_command & 0x7F);

	mac2str (mac, macstr);
	Serial.println ();
	Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));
	if (_command == nodeMessageType_t::DOWNSTREAM_DATA_GET)
		commandStr = "GET";
	else if (_command == nodeMessageType_t::DOWNSTREAM_DATA_SET)
		commandStr = "SET";
	else
		return;

	Serial.printf ("%s Command %s\n", broadcast ? "Broadcast" : "Unicast", commandStr.c_str ());
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
		Serial.println ();
		break;
	default:
		DEBUG_WARN ("Payload encoding %d is not (yet) supported", payloadEncoding);
	}
}

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();

#ifdef ESP32
	// Turn-off the 'brownout detector' to avoid random restarts during wake up,
	// normally due to bad quality regulator on board
	WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0);
#endif

	EnigmaIOTNode.setLed (BLUE_LED);
	//pinMode (BLUE_LED, OUTPUT);
	//digitalWrite (BLUE_LED, HIGH); // Turn on LED
	EnigmaIOTNode.setResetPin (RESET_PIN);
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);
	EnigmaIOTNode.enableClockSync ();
	EnigmaIOTNode.enableBroadcast ();

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

void showTime () {
	//const int time_freq = 10000;
	
	tm timeinfo;
	static time_t displayTime;
	
	if (EnigmaIOTNode.hasClockSync()) {
		setenv ("TZ", TZINFO, 1);
		displayTime = millis ();
		time_t local_time_ms = EnigmaIOTNode.clock ();
		//local_time_ms /= 1000;
		time_t local_time = EnigmaIOTNode.unixtime ();
		localtime_r (&local_time, &timeinfo);
		//Serial.printf ("Timestamp ms: %lld\n", local_time_ms);
		//Serial.printf ("Timestamp sec: %ld\n", local_time);
		Serial.printf ("%02d/%02d/%04d %02d:%02d:%02d\n",
					   timeinfo.tm_mday,
					   timeinfo.tm_mon + 1,
					   timeinfo.tm_year + 1900,
					   timeinfo.tm_hour,
					   timeinfo.tm_min,
					   timeinfo.tm_sec);
	} else {
		Serial.printf ("Time not sync'ed\n");
	}

}

void loop () {

	EnigmaIOTNode.handle ();

	CayenneLPP msg (20);

	static time_t lastSensorData;
	static const time_t SENSOR_PERIOD = 10000;
	if (millis () - lastSensorData > SENSOR_PERIOD) {
		lastSensorData = millis ();
		showTime ();
		// Read sensor data
#ifdef ESP8266
		msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
		Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
#elif defined ESP32
		msg.addAnalogInput (0, (float)(analogRead (ADC1_CHANNEL_0_GPIO_NUM) * 3.6 / 4096));
		Serial.printf ("Vcc: %f V\n", (float)(analogRead (ADC1_CHANNEL_0_GPIO_NUM) * 3.6 / 4096));
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
