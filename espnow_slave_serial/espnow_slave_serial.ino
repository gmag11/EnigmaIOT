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

bool serverHello (byte *key) {
	byte buffer[KEY_LENGTH + 5];
	uint32_t crc32;

	if (!key) {
		return false;
	}

	buffer[0] = 0xfe; // Server hello message

	for (int i = 0; i < KEY_LENGTH; i++) {
		buffer[i + 1] = key[i];
	}

	crc32 = CRC32::calculate (buffer, KEY_LENGTH + 1);
	Serial.printf ("CRC32 = 0x%08X\n", crc32);

	// int is little indian mode on ESP platform
	uint32_t *crcField = (uint32_t*)&(buffer[KEY_LENGTH + 1]);

	*crcField = crc32;
	printHexBuffer (buffer, KEY_LENGTH + 5);

	WifiEspNow.send (peer, buffer, KEY_LENGTH + 5);
}

void clientHello (const uint8_t mac[6], const uint8_t* buf, size_t count) {

}

void manageMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) {
	Serial.printf ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	//Serial.print ("Received data: ");
	//printHexBuffer ((byte *)buf, count);
	flashBlue = true;
	if (count <= 1) {
		return;
	}

	switch (buf[0]) {
	case 0xFF:
		Serial.println ("Recibido Client Hello");
		WifiEspNow.addPeer (mac);
		clientHello (mac, &(buf[1]), count - 1);
	}
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