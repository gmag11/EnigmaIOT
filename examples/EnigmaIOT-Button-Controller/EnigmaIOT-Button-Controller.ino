/**
  * @file EnigmaIOT-Button-Controller.ino
  * @version 0.9.7
  * @date 04/02/2021
  * @author German Martin
  * @brief Node template for easy custom node creation
  */

#if !defined ESP8266 && !defined ESP32
#error Node only supports ESP8266 or ESP32 platform
#endif

#include <Arduino.h>
#include <EnigmaIOTjsonController.h>
#include <FailSafe.h>
#include "ButtonController.h" // <-- Include here your controller class header

#include <EnigmaIOTNode.h>
#include <espnow_hal.h>
#include <CayenneLPP.h>
#include <ArduinoJson.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Hash.h>
#elif defined ESP32
#include <WiFi.h>
#include <Update.h>
#include <driver/adc.h>
#include "esp_wifi.h"
#endif
#include <ArduinoJson.h>
#include <Curve25519.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <DNSServer.h>

#define SLEEPY 0 // Set it to 1 if your node should sleep after sending data

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // ESP32 boards normally have a LED in GPIO3 or GPIO5
#endif // !LED_BUILTIN

// If you do need serial for your project you must disable serial debug by commenting next line
#define USE_SERIAL // Don't forget to set DEBUG_LEVEL to NONE if serial is disabled

#define BLUE_LED LED_BUILTIN // You can set a different LED pin here. -1 means disabled

EnigmaIOTjsonController* controller; // Generic controller is refferenced here. You do not need to modify it

#define RESET_PIN 13 // You can set a different configuration reset pin here. Check for conflicts with used pins.

const time_t BOOT_FLAG_TIMEOUT = 10000; // Time in ms to reset flag
const int MAX_CONSECUTIVE_BOOT = 3; // Number of rapid boot cycles before enabling fail safe mode
const int LED = LED_BUILTIN; // Number of rapid boot cycles before enabling fail safe mode
const int FAILSAFE_RTC_ADDRESS = 0; // If you use RTC memory adjust offset to not overwrite other data

// Called when node is connected to gateway. You don't need to do anything here usually
void connectEventHandler () {
	controller->connectInform ();
	DEBUG_WARN ("Connected");
}

// Called when node is unregistered from gateway. You don't need to do anything here usually
void disconnectEventHandler (nodeInvalidateReason_t reason) {
	DEBUG_WARN ("Disconnected. Reason %d", reason);
}

// Called to route messages to EnitmaIOTNode class. Do not modify
bool sendUplinkData (const uint8_t* data, size_t len, nodePayloadEncoding_t payloadEncoding) {
	return EnigmaIOTNode.sendData (data, len, payloadEncoding);
}

// Called to route incoming messages to your code. Do not modify
void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) {
	if (controller->processRxCommand (mac, buffer, length, command, payloadEncoding)) {
		DEBUG_INFO ("Command processed");
	} else {
		DEBUG_WARN ("Command error");
	}
}

// Do not modify
void wifiManagerExit (boolean status) {
	controller->configManagerExit (status);
}

// Do not modify
void wifiManagerStarted () {
	controller->configManagerStart ();
}

void setup () {

#ifdef USE_SERIAL
	Serial.begin (115200);
	delay (1000);
	Serial.println ();
#endif

    FailSafe.checkBoot (MAX_CONSECUTIVE_BOOT, LED, FAILSAFE_RTC_ADDRESS); // Parameters are optional
    if (FailSafe.isActive ()) { // Skip all user setup if fail safe mode is activated
        return;
    }

	controller = (EnigmaIOTjsonController*)new CONTROLLER_CLASS_NAME (); // Use your class name here

	EnigmaIOTNode.setLed (BLUE_LED); // Set communication LED
	EnigmaIOTNode.setResetPin (RESET_PIN); // Set reset pin
	EnigmaIOTNode.onConnected (connectEventHandler); // Configure registration handler
	EnigmaIOTNode.onDisconnected (disconnectEventHandler); // Configure unregistration handler
	EnigmaIOTNode.onDataRx (processRxData); // Configure incoming data handler
	EnigmaIOTNode.enableClockSync (false); // Set to true if you need this node to get its clock syncronized with gateway
	EnigmaIOTNode.onWiFiManagerStarted (wifiManagerStarted);
	EnigmaIOTNode.onWiFiManagerExit (wifiManagerExit);
	EnigmaIOTNode.enableBroadcast ();

	if (!controller->loadConfig ()) { // Trigger custom configuration loading
		DEBUG_WARN ("Error reading config file");
		if (FILESYSTEM.format ())
			DEBUG_WARN ("Filesystem Formatted");
	}

	EnigmaIOTNode.begin (&Espnow_hal, NULL, NULL, true, SLEEPY == 1); // Start EnigmaIOT communication

	uint8_t macAddress[ENIGMAIOT_ADDR_LEN];
	// Set Address using internal MAC Address. Do not modify
#ifdef ESP8266
	if (wifi_get_macaddr (STATION_IF, macAddress))
#elif defined ESP32
	if ((esp_wifi_get_mac (WIFI_IF_STA, macAddress) == ESP_OK))
#endif
	{
		EnigmaIOTNode.setNodeAddress (macAddress);
		DEBUG_DBG ("Node address set to %s", mac2str (macAddress));
	} else {
		DEBUG_WARN ("Node address error");
	}

	controller->sendDataCallback (sendUplinkData); // Listen for data from controller class
	controller->setup (&EnigmaIOTNode);			   // Start controller class

#if SLEEPY == 1
	EnigmaIOTNode.sleep ();
#endif

	DEBUG_DBG ("END setup");
}

void loop () {
    FailSafe.loop (BOOT_FLAG_TIMEOUT); // Use always this line

    if (FailSafe.isActive ()) { // Skip all user loop code if Fail Safe mode is active
        return;
    }

	controller->loop (); // Loop controller class
	EnigmaIOTNode.handle (); // Mantain EnigmaIOT connection
}
