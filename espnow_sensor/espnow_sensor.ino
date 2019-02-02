#include <WifiEspNow.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "cryptModule.h"
#include "helperFunctions.h"
#include <CRC32.h>

byte gateway[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void initEspNow () {
    bool ok = WifiEspNow.begin ();
    if (!ok) {
        Serial.println ("WifiEspNow.begin() failed");
        ESP.restart ();
    }
	WifiEspNow.addPeer (gateway);
}

bool clientHello (byte *key) {
	byte buffer[KEY_LENGTH + 5];
	uint32_t crc32;

	if (!key) {
		return false;
	}

	buffer[0] = 0xff; // Client hello message

	for (int i = 0; i < KEY_LENGTH; i++) {
		buffer[i + 1] = key[i];
	}

	crc32 = CRC32::calculate (buffer, KEY_LENGTH + 1);
	Serial.printf ("CRC32 = 0x%08X\n", crc32);

	// int is little indian mode on ESP platform
	uint32_t *crcField = (uint32_t*)&(buffer[KEY_LENGTH + 1]);

	*crcField = crc32;
	printHexBuffer (buffer, KEY_LENGTH + 5);

	WifiEspNow.send (gateway, buffer, KEY_LENGTH + 5);
}

void setup () {
    //***INICIALIZACIÓN DEL PUERTO SERIE***//
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    initEspNow ();

    Crypto.getDH1 ();

	clientHello (Crypto.getPubDHKey());
}

void loop () {

}