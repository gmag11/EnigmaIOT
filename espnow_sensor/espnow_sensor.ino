#include <WifiEspNow.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include <CRC32.h>

enum status_t {
	INIT,
	WAIT_FOR_SERVER_HELLO,
	WAIT_FOR_CIPHER_FINISHED,
	WAIT_FOR_KEY_EXCH_FINISHED,
	WAIT_FOR_DOWNLINK,
	WAIT_FOR_SENSOR_DATA,
	SLEEP
};

enum messageType_t {
	SENSOR_DATA = 0x01,
	CLIENT_HELLO = 0xFF,
	SERVER_HELLO = 0xFE,
	KEY_EXCHANGE_FINISHED = 0xFD,
	CYPHER_FINISHED = 0xFC,
	INVALIDATE_KEY = 0xFB
};

struct node_instance {
	uint8_t mac[6];
	uint16_t nodeId;
	uint8_t key[32];
	time_t lastMessage;
	bool keyInvalid;
};

typedef struct node_instance node_t;

byte gateway[6] = { 0x5E, 0xCF, 0x7F, 0x80, 0x34, 0x75 };
enum status_t state = INIT;
uint8_t key[KEY_LENGTH];

void initEspNow () {
    bool ok = WifiEspNow.begin ();
    if (!ok) {
        DEBUG_MSG ("WifiEspNow.begin() failed\n");
        ESP.restart ();
    }
	WifiEspNow.addPeer (gateway);
	WifiEspNow.onReceive (manageMessage, NULL);
}

bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc) {
	uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_MSG ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X\n", _crc, *crc);
	return (_crc == *crc);
}

bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count) {
	uint8_t myPublicKey[KEY_LENGTH];

	if (!checkCRC (buf, count - 4, (uint32_t*)(buf + count - 4))) {
        DEBUG_MSG ("Wrong CRC\n");
		return false;
	}

	//memcpy (node.mac, mac, 6);
	memcpy (key, &(buf[1]), KEY_LENGTH);
	//memcpy (myPublicKey, Crypto.getPubDHKey (), KEY_LENGTH);
	Crypto.getDH2 (key);
	DEBUG_MSG ("Node key: ");
	printHexBuffer (key, KEY_LENGTH);

	return true;
}

void manageMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) {
    DEBUG_MSG ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_MSG ("Received data: ");
	printHexBuffer ((byte *)buf, count);
    DEBUG_MSG ("Received CRC: ");
	printHexBuffer ((byte *)(buf + count - 4), 4);
	if (count <= 1) {
		return;
	}

	switch (buf[0]) {
	case SERVER_HELLO:
        DEBUG_MSG ("Server Hello received\n");
		processServerHello (mac, buf, count);
	}
}

bool clientHello (byte *key) {
	byte buffer[KEY_LENGTH + 5];
	uint32_t crc32;

	if (!key) {
		return false;
	}

	buffer[0] = CLIENT_HELLO; // Client hello message

	for (int i = 0; i < KEY_LENGTH; i++) {
		buffer[i + 1] = key[i];
	}

	crc32 = CRC32::calculate (buffer, KEY_LENGTH + 1);
    DEBUG_MSG ("CRC32 = 0x%08X\n", crc32);

	// int is little indian mode on ESP platform
	uint32_t *crcField = (uint32_t*)&(buffer[KEY_LENGTH + 1]);

	*crcField = crc32;
    DEBUG_MSG ("lient Hello message: ");
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