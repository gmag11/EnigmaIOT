#include <WifiEspNow.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include <CRC32.h>
#include "Node.h"


enum messageType_t {
	SENSOR_DATA = 0x01,
	CLIENT_HELLO = 0xFF,
	SERVER_HELLO = 0xFE,
	KEY_EXCHANGE_FINISHED = 0xFD,
	CYPHER_FINISHED = 0xFC,
	INVALIDATE_KEY = 0xFB
};

byte gateway[6] = { 0x5E, 0xCF, 0x7F, 0x80, 0x34, 0x75 };

Node node;

#define BLUE_LED 2
bool flashBlue = false;

void initEspNow () {
    bool ok = WifiEspNow.begin ();
    if (!ok) {
        DEBUG_ERROR ("WifiEspNow.begin() failed");
        ESP.restart ();
    }
	WifiEspNow.addPeer (gateway);
	WifiEspNow.onReceive (manageMessage, NULL);
}

bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc){
    uint32 recvdCRC;

    memcpy (&recvdCRC, crc, sizeof (uint32_t));
    //DEBUG_VERBOSE ("Received CRC32: 0x%08X", *crc);
	uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X Length: %d", _crc, recvdCRC, count);
	return (_crc == recvdCRC);
}

bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count) {
	uint8_t myPublicKey[KEY_LENGTH];
    uint32_t crc = *(uint32_t*)(buf + count - 4);
    uint8_t key[KEY_LENGTH];

	if (!checkCRC (buf, count - CRC_LENGTH, &crc)) {
        DEBUG_WARN ("Wrong CRC");
		return false;
	}

	//memcpy (node.mac, mac, 6);
    
	memcpy (key, &(buf[1]), KEY_LENGTH);
	//memcpy (myPublicKey, Crypto.getPubDHKey (), KEY_LENGTH);
	Crypto.getDH2 (key);
    node.setEncryptionKey (key);
    DEBUG_INFO ("Node key: %s", printHexBuffer (node.getEncriptionKey(), KEY_LENGTH));

	return true;
}

bool processCypherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count) {
    uint16_t nodeId;

    DEBUG_VERBOSE ("%s Length: %d", printHexBuffer ((byte *)buf, count), count);
    if (!checkCRC (buf, count - 4, (uint32_t*)(buf + count - 4))) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }
    memcpy (&nodeId, buf + 1, sizeof (uint16_t));
    node.setNodeId (nodeId);
    DEBUG_VERBOSE ("Node ID: %u", node.getNodeId());
    return true;
}

bool keyExchangeFinished () {
	byte buffer[/*RANDOM_LENGTH + 5*/16]; //1 block
	uint32_t crc32;
	uint32_t nonce;

	memset (buffer, 0, 16);

	buffer[0] = KEY_EXCHANGE_FINISHED; // Client hello message

	nonce = Crypto.random ();

	memcpy (buffer + 1, &nonce, RANDOM_LENGTH);

	crc32 = CRC32::calculate (buffer, RANDOM_LENGTH + 1);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	// int is little indian mode on ESP platform
	uint32_t *crcField = (uint32_t*)&(buffer[RANDOM_LENGTH + 1]);

	*crcField = crc32;
	DEBUG_VERBOSE ("Key Exchange Finished message: %s", printHexBuffer (buffer, 16));

	Crypto.encryptBuffer (buffer, buffer, /*RANDOM_LENGTH + 5*/16, node.getEncriptionKey());

	DEBUG_VERBOSE ("Encripted Key Exchange Finished message: %s", printHexBuffer (buffer, 16));

	return WifiEspNow.send (gateway, buffer, 16);
}

void manageMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) {
    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s",printHexBuffer ((byte *)buf, count));
    flashBlue = true;
    if ((count % 16) == 0) { // Encrypted frames have to be n times 16 bytes long
        Crypto.decryptBuffer ((uint8_t *)buf, (uint8_t *)buf, count, node.getEncriptionKey ());
        DEBUG_VERBOSE ("Decrypted data: %s", printHexBuffer ((byte *)buf, count));
    } else if (buf[0] != SERVER_HELLO) { // Only valid unencrypted message is Server Hello
        DEBUG_WARN ("Non valid unencrypted message");
        return;
    }
	//DEBUG_VERBOSE ("Received CRC: %s", printHexBuffer ((byte *)(buf + count - 4), 4));
	if (count <= 1) {
		return;
	}

	switch (buf[0]) {
	case SERVER_HELLO:
        DEBUG_INFO ("Server Hello received");
        node.setLastMessageTime (millis ());
        if (node.getStatus () == WAIT_FOR_SERVER_HELLO) {
            if (processServerHello (mac, buf, count)) {
                keyExchangeFinished ();
                node.setStatus (WAIT_FOR_CIPHER_FINISHED);
            } else {
                node.reset ();
                node.setStatus (UNREGISTERED);
            } 
        } else {
            node.reset ();
            node.setStatus (UNREGISTERED);
        }
        break;
    case CYPHER_FINISHED:
        DEBUG_INFO ("Cypher Finished received");
        node.setLastMessageTime (millis ());
        if (node.getStatus () == WAIT_FOR_CIPHER_FINISHED) {
            if (processCypherFinished (mac, buf, 3 + RANDOM_LENGTH + CRC_LENGTH)) {
                // mark node as registered
                node.setKeyValid (true);
                node.setStatus (REGISTERED);
                node.printToSerial ();
                // TODO: Store node data on EEPROM, SPIFFS or RTCMEM
            } else {
                node.reset ();
                node.setStatus (UNREGISTERED);
            }
        } else {
            node.reset ();
            node.setStatus (UNREGISTERED);
        }
        break;
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

    node.setStatus (WAIT_FOR_SERVER_HELLO);
	return WifiEspNow.send (gateway, buffer, KEY_LENGTH + 5);
}

void setup () {
    //***INICIALIZACIÓN DEL PUERTO SERIE***//
    Serial.begin (115200); Serial.println (); Serial.println ();

    pinMode (BLUE_LED, OUTPUT);
    digitalWrite (BLUE_LED, HIGH);

    initWiFi ();
    initEspNow ();

    Crypto.getDH1 ();
    node.setStatus (INIT);
	clientHello (Crypto.getPubDHKey());
}

void loop () {
#define LED_PERIOD 100
    static unsigned long blueOntime;

    if (flashBlue) {
        blueOntime = millis ();
        digitalWrite (BLUE_LED, LOW);
        flashBlue = false;
    }

    if (!digitalRead (BLUE_LED) && millis () - blueOntime > LED_PERIOD) {
        digitalWrite (BLUE_LED, HIGH);
    }
}