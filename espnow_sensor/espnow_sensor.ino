#if defined(ESP8266)
extern "C" {
#include <espnow.h>
}
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
    if (esp_now_init () != 0) {
        Serial.println ("Protocolo ESP-NOW no inicializado...");
        ESP.restart ();
        delay (1);
    }
    esp_now_set_self_role (ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_recv_cb (manageMessage);
    esp_now_add_peer (gateway, ESP_NOW_ROLE_SLAVE, 0, NULL, 0);
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
    
    if (count < 1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH) {
        DEBUG_WARN ("Message too short");
        return false;
    }

	if (!checkCRC (buf, count - CRC_LENGTH, &crc)) {
        DEBUG_WARN ("Wrong CRC");
		return false;
	}

	//memcpy (node.mac, mac, 6);
    
	memcpy (key, buf + 1 + IV_LENGTH, KEY_LENGTH);
	Crypto.getDH2 (key);
    node.setEncryptionKey (key);
    DEBUG_INFO ("Node key: %s", printHexBuffer (node.getEncriptionKey(), KEY_LENGTH));

	return true;
}

bool processCipherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count) {
    uint16_t nodeId;
    uint8_t *iv;
    uint32_t crc;

    if (count < (1 + IV_LENGTH + sizeof(uint16_t) + RANDOM_LENGTH + CRC_LENGTH)) {
        DEBUG_WARN ("Wrong message length --> Required: %d Received: %d", 
            1 + IV_LENGTH + sizeof (uint16_t) + RANDOM_LENGTH + CRC_LENGTH, count);
        return false;
    }

    iv = (uint8_t *)(buf + 1);
    uint8_t *crypt_buf = (uint8_t *)(buf + 1 + IV_LENGTH);
    Crypto.decryptBuffer (crypt_buf, crypt_buf, sizeof (uint16_t) + RANDOM_LENGTH + CRC_LENGTH, iv, IV_LENGTH, node.getEncriptionKey (), KEY_LENGTH);
    DEBUG_VERBOSE ("Decripted Cipher Finished message: %s", printHexBuffer ((byte *)buf, 1 + IV_LENGTH + sizeof (uint16_t) + RANDOM_LENGTH + CRC_LENGTH));

    memcpy (&crc, buf + 1 + IV_LENGTH + sizeof (uint16_t) + RANDOM_LENGTH, CRC_LENGTH);

    if (!checkCRC (buf, count - 4, &crc)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }
    memcpy (&nodeId, buf + 1 + IV_LENGTH, sizeof (uint16_t));
    node.setNodeId (nodeId);
    DEBUG_VERBOSE ("Node ID: %u", node.getNodeId());
    return true;
}

bool keyExchangeFinished () {
	byte buffer[1 + IV_LENGTH + RANDOM_LENGTH + CRC_LENGTH]; //1 block
	uint32_t crc32;
	uint32_t nonce;
    uint8_t *iv;

	memset (buffer, 0, 1 + IV_LENGTH + RANDOM_LENGTH + CRC_LENGTH);

	buffer[0] = KEY_EXCHANGE_FINISHED; // Client hello message

    iv = CryptModule::random (buffer + 1, IV_LENGTH);
    DEBUG_VERBOSE ("IV: %s", printHexBuffer (buffer + 1, IV_LENGTH));

	nonce = Crypto.random ();

	memcpy (buffer + 1 + IV_LENGTH, (uint8_t *)&nonce, RANDOM_LENGTH);

	crc32 = CRC32::calculate (buffer, 1 + IV_LENGTH + RANDOM_LENGTH);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	// int is little indian mode on ESP platform
	uint8_t *crcField = (uint8_t*)(buffer + 1 + IV_LENGTH + RANDOM_LENGTH);
    memcpy (crcField, (uint8_t *)&crc32, CRC_LENGTH);

	DEBUG_VERBOSE ("Key Exchange Finished message: %s", printHexBuffer (buffer, 1 + IV_LENGTH + RANDOM_LENGTH + CRC_LENGTH));

    uint8_t *crypt_buf = buffer + 1 + IV_LENGTH;

	Crypto.encryptBuffer (crypt_buf, crypt_buf, RANDOM_LENGTH + CRC_LENGTH, iv, IV_LENGTH, node.getEncriptionKey(), KEY_LENGTH);

	DEBUG_VERBOSE ("Encripted Key Exchange Finished message: %s", printHexBuffer (buffer, 1 + IV_LENGTH + RANDOM_LENGTH + CRC_LENGTH));
    DEBUG_INFO (" -------> KEY_EXCHANGE_FINISHED");
	return esp_now_send (gateway, buffer, 1 + IV_LENGTH + RANDOM_LENGTH + CRC_LENGTH) == 0;
}

void manageMessage (uint8_t *mac, uint8_t* buf, uint8_t count/*, void* cbarg*/) {
    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s",printHexBuffer ((byte *)buf, count));
    flashBlue = true;

    if (count <= 1) {
        return;
    }

	switch (buf[0]) {
	case SERVER_HELLO:
        DEBUG_INFO (" <------- SERVER HELLO");
        node.setLastMessageTime (millis ());
        if (node.getStatus () == WAIT_FOR_SERVER_HELLO) {
            if (processServerHello (mac, buf, count)) {
                keyExchangeFinished ();
                node.setStatus (WAIT_FOR_CIPHER_FINISHED);
            } else {
                node.reset ();
            } 
        } else {
            node.reset ();
        }
        break;
    case CYPHER_FINISHED:
        DEBUG_INFO ("Cypher Finished received");
        node.setLastMessageTime (millis ());
        if (node.getStatus () == WAIT_FOR_CIPHER_FINISHED) {
            DEBUG_INFO (" <------- CIPHER_FINISHED");
            if (processCipherFinished (mac, buf, count)) {
                // mark node as registered
                node.setKeyValid (true);
                node.setStatus (REGISTERED);
#if DEGUG_LEVEL >= INFO
                node.printToSerial (&DEBUG_ESP_PORT);
#endif
                // TODO: Store node data on EEPROM, SPIFFS or RTCMEM
            } else {
                node.reset ();
            }
        } else {
            node.reset ();
        }
        break;
	}
}

bool clientHello (uint8_t *key) {
	uint8_t buffer[1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH];
	uint32_t crc32;

	if (!key) {
		return false;
	}

	buffer[0] = CLIENT_HELLO; // Client hello message

    CryptModule::random (buffer + 1, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (buffer +1, IV_LENGTH));

	for (int i = 0; i < KEY_LENGTH; i++) {
		buffer[i + 1 + IV_LENGTH] = key[i];
	}

	crc32 = CRC32::calculate (buffer, 1 + IV_LENGTH + KEY_LENGTH);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	// int is little indian mode on ESP platform
	uint8_t *crcField = (uint8_t*)(buffer + 1 + IV_LENGTH + KEY_LENGTH);

	//*crcField = crc32;
    memcpy (crcField, &crc32, CRC_LENGTH);

	DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer (buffer, 1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH));

    node.setStatus (WAIT_FOR_SERVER_HELLO);

    DEBUG_INFO (" -------> CLIENT HELLO");

	//return WifiEspNow.send (gateway, buffer, 1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH);
    return esp_now_send (gateway, buffer, 1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH) == 0;
}

bool dataMessage (uint8_t *data, size_t len) {
    uint8_t buffer[200];
    uint32_t crc32;
    uint32_t nonce;
    uint16_t nodeId = node.getNodeId ();

    if (!data) {
        return false;
    }

    buffer[0] = SENSOR_DATA;

    CryptModule::random (buffer + 1, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (buffer + 1, IV_LENGTH));

    memcpy (buffer + 1 + IV_LENGTH + 2, &nodeId, sizeof(uint16_t));

    nonce = Crypto.random ();

    memcpy (buffer + 1 + IV_LENGTH + 2 + 2, (uint8_t *)&nonce, RANDOM_LENGTH);

    memcpy (buffer + 1 + IV_LENGTH + 2 + 2 + RANDOM_LENGTH, data, len);

    uint16_t packet_length = 1 + IV_LENGTH + 2 + 2 + RANDOM_LENGTH + len;

    memcpy (buffer + 1 + IV_LENGTH, &packet_length, sizeof (uint16_t));

    crc32 = CRC32::calculate (buffer, packet_length);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    // int is little indian mode on ESP platform
    uint8_t *crcField = (uint8_t*)(buffer + packet_length);

    memcpy (crcField, &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer (buffer, 1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH));
    
    return esp_now_send (gateway, buffer, packet_length + CRC_LENGTH) == 0;
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
    uint8_t macAddress[6];
    if (wifi_get_macaddr (0, macAddress)) {
        node.setMacAddress (macAddress);
    }
    clientHello (Crypto.getPubDHKey());
}

void loop () {
#define LED_PERIOD 100
    static unsigned long blueOntime;
    //static time_t lastMessage;
    //static char *message = "Hello World!!!";

    if (flashBlue) {
        blueOntime = millis ();
        digitalWrite (BLUE_LED, LOW);
        flashBlue = false;
    }

    if (!digitalRead (BLUE_LED) && millis () - blueOntime > LED_PERIOD) {
        digitalWrite (BLUE_LED, HIGH);
    }

    /*if (millis () - lastMessage > 5000) {
        lastMessage = millis ();
        DEBUG_INFO ("Trying to send");
        if (node.getStatus() == REGISTERED && node.isKeyValid()) {
            DEBUG_INFO ("Sending data: %s", printHexBuffer((byte*)message,strlen(message)));
            dataMessage ((uint8_t *)message, strlen (message));
        }
    }*/
}