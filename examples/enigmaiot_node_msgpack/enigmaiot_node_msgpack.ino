/**
  * @file enigmaiot_node.ino
  * @version 0.9.6
  * @date 10/12/2020
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW that uses MessagePack as payload encoding
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
#include <AsyncTCP.h> // Comment to compile for ESP8266
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
#ifdef ESP8266
#define LED_BUILTIN 2 // ESP8266 boards normally have a LED in GPIO2
#else
#define LED_BUILTIN 5 // ESP32 boards normally have a LED in GPIO2 or GPIO5
#endif
#endif // !LED_BUILTIN

#define BLUE_LED LED_BUILTIN
constexpr auto RESET_PIN = -1;

#ifdef ESP8266
ADC_MODE (ADC_VCC);
#endif

void connectEventHandler () {
	Serial.println ("Registered");
}

void disconnectEventHandler (nodeInvalidateReason_t reason) {
	Serial.printf ("Unregistered. Reason: %d\n", reason);
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t encoding) {
	char macstr[ENIGMAIOT_ADDR_LEN * 3];
	String commandStr;
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

	CayenneLPP lpp (MAX_DATA_PAYLOAD_SIZE);
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
	default:
		DEBUG_WARN ("Non supported encoding; %d", encoding);
	}
}

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();
	time_t start = millis ();

#ifdef ESP32
	// Turn-off the 'brownout detector' to avoid random restarts during wake up,
	// normally due to bad quality regulator on board
	WRITE_PERI_REG (RTC_CNTL_BROWN_OUT_REG, 0);
#endif

	EnigmaIOTNode.setLed (BLUE_LED);
	EnigmaIOTNode.setResetPin (RESET_PIN);
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);

	EnigmaIOTNode.begin (&Espnow_hal);

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

	// Put here your code to read sensor and compose buffer
	const size_t capacity = JSON_OBJECT_SIZE (5);
	DynamicJsonDocument json (capacity);

#ifdef ESP8266
	json["V"] = (float)(ESP.getVcc ()) / 1000;
#elif defined ESP32
	json["V"] = (float)(analogRead (ADC1_CHANNEL_0_GPIO_NUM) * 3.6 / 4096);
#endif
	json["tem"] = 203;
	json["din"] = 123;
	json["pres"] = 1007;
	json["curr"] = 2.43;

	int len = measureMsgPack (json) + 1;
	uint8_t* buffer = (uint8_t*)malloc (len);
	len = serializeMsgPack (json, (char*)buffer, len);

#ifdef ESP8266
	Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
#elif defined ESP32
	Serial.printf ("Vcc: %f\n", (float)(analogRead (ADC1_CHANNEL_0_GPIO_NUM) * 3.6 / 4096));
#endif
	Serial.printf ("Message Len %d\n", len);
	// End of user code

	Serial.printf ("Trying to send: %s\n", printHexBuffer (buffer, len));

	// Send buffer data
	if (!EnigmaIOTNode.sendData (buffer, len, MSG_PACK)) {
		Serial.println ("---- Error sending data");
	} else {
		Serial.println ("---- Data sent");
	}
	Serial.printf ("Total time: %lu ms\n", millis () - start);

	free (buffer);

	// Go to sleep
	EnigmaIOTNode.sleep ();
}

void loop () {

	EnigmaIOTNode.handle ();

}
