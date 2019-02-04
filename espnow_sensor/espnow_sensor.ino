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
        DEBUG_ERROR ("WifiEspNow.begin() failed");
        ESP.restart ();
    }
	WifiEspNow.addPeer (gateway);
	WifiEspNow.onReceive (manageMessage, NULL);
}

bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc) {
	uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X", _crc, *crc);
	return (_crc == *crc);
}

bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count) {
	uint8_t myPublicKey[KEY_LENGTH];

	if (!checkCRC (buf, count - 4, (uint32_t*)(buf + count - 4))) {
        DEBUG_WARN ("Wrong CRC");
		return false;
	}

	//memcpy (node.mac, mac, 6);
	memcpy (key, &(buf[1]), KEY_LENGTH);
	//memcpy (myPublicKey, Crypto.getPubDHKey (), KEY_LENGTH);
	Crypto.getDH2 (key);
	DEBUG_INFO ("Node key: %s", printHexBuffer (key, KEY_LENGTH));

	return true;
}

bool keyExchangeFinished () {
	byte buffer[RANDOM_LENGTH + 5];
	uint32_t crc32;
	uint32_t nonce;

	buffer[0] = KEY_EXCHANGE_FINISHED; // Client hello message

	nonce = Crypto.random ();

	memcpy (buffer + 1, &nonce, RANDOM_LENGTH);

	crc32 = CRC32::calculate (buffer, RANDOM_LENGTH + 1);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	// int is little indian mode on ESP platform
	uint32_t *crcField = (uint32_t*)&(buffer[RANDOM_LENGTH + 1]);

	*crcField = crc32;
	DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer (buffer, RANDOM_LENGTH + 5));

	Crypto.encryptBuffer (buffer+1, buffer+1, RANDOM_LENGTH + 4, key);

	DEBUG_VERBOSE ("Encripted Key Exchange Finished message: %s", printHexBuffer (buffer, RANDOM_LENGTH + 5));

	return WifiEspNow.send (gateway, buffer, RANDOM_LENGTH + 5);
}

void manageMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) {
    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s",printHexBuffer ((byte *)buf, count));
	DEBUG_VERBOSE ("Received CRC: %s", printHexBuffer ((byte *)(buf + count - 4), 4));
	if (count <= 1) {
		return;
	}

	switch (buf[0]) {
	case SERVER_HELLO:
        DEBUG_INFO ("Server Hello received");
		if (processServerHello (mac, buf, count)) {
			keyExchangeFinished ();
		}
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
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	// int is little indian mode on ESP platform
	uint32_t *crcField = (uint32_t*)&(buffer[KEY_LENGTH + 1]);

	*crcField = crc32;
	DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer (buffer, KEY_LENGTH + 5));

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