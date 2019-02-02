#include <WifiEspNow.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include <CRC32.h>

#define BLUE_LED 2
#define RED_LED 16
bool flashRed = false;
bool flashBlue = false;

void manageMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) {
	Serial.printf ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	Serial.print ("Received data: ");
	printHexBuffer ((byte *)buf, count);
	flashBlue = true;
}

void initEspNow () {
	bool ok = WifiEspNow.begin ();
	if (!ok) {
		Serial.println ("WifiEspNow.begin() failed");
		ESP.restart ();
	}
	WifiEspNow.onReceive (manageMessage, NULL);
}

void setup () {
	//***INICIALIZACIÓN DEL PUERTO SERIE***//
	Serial.begin (115200); Serial.println (); Serial.println ();

	pinMode (BLUE_LED, OUTPUT);
	digitalWrite (BLUE_LED, HIGH);
	pinMode (RED_LED, OUTPUT);
	digitalWrite (RED_LED, HIGH);

	initWiFi ();
	initEspNow ();

	Crypto.getDH1 ();

	//clientHello (Crypto.getPubDHKey ());
}

void loop () {
#define LED_PERIOD 100
	static unsigned long blueOntime;
	static unsigned long redOntime;

	if (flashBlue) {
		blueOntime = millis ();
		digitalWrite (BLUE_LED, LOW);
		flashBlue = false;
	}

	if (!digitalRead(BLUE_LED) && millis () - blueOntime > LED_PERIOD) {
		digitalWrite (BLUE_LED, HIGH);
	}
}