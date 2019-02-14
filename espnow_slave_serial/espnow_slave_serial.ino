#include <WifiEspNow.h>
#include "NodeList.h"
extern "C" {
#include <espnow.h>
}
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

enum messageType_t {
	SENSOR_DATA = 0x01,
	CLIENT_HELLO = 0xFF,
	SERVER_HELLO = 0xFE,
	KEY_EXCHANGE_FINISHED = 0xFD,
	CYPHER_FINISHED = 0xFC,
	INVALIDATE_KEY = 0xFB
};

uint8_t myPublicKey[KEY_LENGTH];

node_t node;

NodeList nodelist;

bool serverHello (uint8_t *key, Node *node) {
    /*
    * ------------------------------------------------------
    *| msgType (1) | random (16) | DH Kslave (32) | CRC (4) |
    * ------------------------------------------------------
    */

    uint8_t msgType_idx = 0;
    uint8_t iv_idx =      1;
    uint8_t pubKey_idx =  iv_idx     + IV_LENGTH;
    uint8_t crc_idx =     pubKey_idx + KEY_LENGTH;

#define SHMSG_LEN (1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH)

    uint8_t buffer[SHMSG_LEN];
	uint32_t crc32;

	if (!key) {
        DEBUG_ERROR ("NULL key");
		return false;
	}

	buffer[msgType_idx] = SERVER_HELLO; // Server hello message
    
    CryptModule::random (&buffer[iv_idx], IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (&buffer[iv_idx], IV_LENGTH));

	for (int i = 0; i < KEY_LENGTH; i++) {
		buffer[pubKey_idx + i] = key[i];
	}

	crc32 = CRC32::calculate (buffer, SHMSG_LEN - CRC_LENGTH);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	// int is little indian mode on ESP platform
	//uint8_t *crcField = buffer + 1 + IV_LENGTH + KEY_LENGTH;

    memcpy (&buffer[crc_idx], &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Server Hello message: %s", printHexBuffer (buffer, SHMSG_LEN));
	flashRed = true;

#ifdef DEBUG_ESP_PORT
    char mac[18];
    mac2str (node->getMacAddress (), mac);
#endif
    DEBUG_INFO (" -------> SERVER_HELLO");
    if (esp_now_send (node->getMacAddress (), buffer, SHMSG_LEN) == 0) {
        DEBUG_INFO ("Server Hello message sent to %s", mac);
        return true;
    } else {
        nodelist.unregisterNode (node);
        DEBUG_ERROR ("Error sending Server Hello message to %s", mac);
        return false;
    }
}

bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc) {
    uint32_t recvdCRC;

    memcpy (&recvdCRC, crc, sizeof (uint32_t)); // Use of memcpy is a must to ensure code does not try to read non memory aligned int
    uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X %s", _crc, recvdCRC, (_crc == recvdCRC) ? "OK" : "FAIL");
    return (_crc == recvdCRC);
}

bool processClientHello (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node) {
    /*
    * -------------------------------------------------------
    *| msgType (1) | random (16) | DH Kmaster (32) | CRC (4) |
    * -------------------------------------------------------
    */
    uint8_t msgType_idx = 0;
    uint8_t iv_idx =      1;
    uint8_t pubKey_idx =  iv_idx     + IV_LENGTH;
    uint8_t crc_idx =     pubKey_idx + KEY_LENGTH;

#define CHMSG_LEN (1 + IV_LENGTH + KEY_LENGTH + CRC_LENGTH)

    uint32_t crc32;

    if (count < CHMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

    memcpy (&crc32, &buf[crc_idx], sizeof(uint32_t));

	if (!checkCRC (buf, count - CRC_LENGTH, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
		return false;
	}

    node->setLastMessageTime(millis ());
    node->setEncryptionKey (&buf[pubKey_idx]);

    Crypto.getDH1 ();
	memcpy (myPublicKey, Crypto.getPubDHKey (), KEY_LENGTH);
    
    if (Crypto.getDH2 (node->getEncriptionKey ())) {
        node->setKeyValid (true);
        node->setStatus (INIT);
        DEBUG_INFO ("Node key: %s", printHexBuffer (node->getEncriptionKey (), KEY_LENGTH));
    } else {
        nodelist.unregisterNode (node);
        char macstr[18];
        mac2str ((uint8_t *)mac, macstr);
        DEBUG_ERROR ("DH2 error with %s", macstr);
        return false;
    }
}

bool cipherFinished (Node *node) {
    /*
    * -----------------------------------------------------------
    *| msgType (1) | IV (16) | nodeId (2) | random (4) | CRC (4) |
    * -----------------------------------------------------------
    */

    uint8_t msgType_idx = 0;
    uint8_t iv_idx =      1;
    uint8_t nodeId_idx =  iv_idx     + IV_LENGTH;
    uint8_t nonce_idx =   nodeId_idx + sizeof (int16_t);
    uint8_t crc_idx =     nonce_idx  + RANDOM_LENGTH;

#define CFMSG_LEN (1 + IV_LENGTH + sizeof(uint16_t) + RANDOM_LENGTH + CRC_LENGTH)

    uint8_t buffer[CFMSG_LEN];

    uint32_t crc32;
    uint32_t nonce;

    //memset (buffer, 0, CFMSG_LEN);

    buffer[msgType_idx] = CYPHER_FINISHED; // Server hello message

    CryptModule::random (&buffer[iv_idx], IV_LENGTH);
    DEBUG_VERBOSE ("IV: %s", printHexBuffer (&buffer[iv_idx], IV_LENGTH));

    uint16_t nodeId = node->getNodeId ();
    memcpy (&buffer[nodeId_idx], &nodeId, sizeof (uint16_t));
    
    nonce = Crypto.random ();

    memcpy (&buffer[nonce_idx], &nonce, RANDOM_LENGTH);

    crc32 = CRC32::calculate (buffer, CFMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    // int is little indian mode on ESP platform
    //uint8_t *crcField = (uint8_t*)(buffer + crc_idx);
    memcpy (&buffer[crc_idx], (uint8_t *)&crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Cipher Finished message: %s", printHexBuffer (buffer, CFMSG_LEN));

    Crypto.encryptBuffer (&buffer[nodeId_idx], &buffer[nodeId_idx], CFMSG_LEN - nodeId_idx, &buffer[iv_idx], IV_LENGTH, node->getEncriptionKey (), KEY_LENGTH);

    DEBUG_VERBOSE ("Encripted Cipher Finished message: %s", printHexBuffer (buffer, CFMSG_LEN));

    flashRed = true;
    DEBUG_INFO (" -------> CYPHER_FINISHED");
    return esp_now_send (node->getMacAddress(), buffer, CFMSG_LEN) == 0;
}

bool processKeyExchangeFinished (const uint8_t mac[6], uint8_t* buf, size_t count, Node *node) {
    /*
    * ----------------------------------------------
    *| msgType (1) | IV (16) | random (4) | CRC (4) |
    * ----------------------------------------------
    */
    uint8_t msgType_idx = 0;
    uint8_t iv_idx =      1;
    uint8_t nonce_idx =  iv_idx + IV_LENGTH;
    uint8_t crc_idx = nonce_idx + RANDOM_LENGTH;

#define KEFMSG_LEN (1 + IV_LENGTH + RANDOM_LENGTH + CRC_LENGTH)

    uint32_t crc;

    if (count < KEFMSG_LEN) {
        DEBUG_WARN ("Wrong message");
        return false;
    }

    //iv = (uint8_t *)(buf + 1);

    //uint8_t *crypt_buf = (uint8_t *)(buf + 1 + IV_LENGTH);
    Crypto.decryptBuffer(&buf[nonce_idx], &buf[nonce_idx], RANDOM_LENGTH + CRC_LENGTH, &buf[iv_idx], IV_LENGTH, node->getEncriptionKey (), KEY_LENGTH);
    DEBUG_VERBOSE ("Decripted Key Exchange Finished message: %s", printHexBuffer (buf, KEFMSG_LEN));

    memcpy (&crc, &buf[crc_idx], CRC_LENGTH);

    if (!checkCRC (buf, count - CRC_LENGTH, &crc)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }
    
    return true;

}

bool processDataMessage (const uint8_t mac[6], uint8_t* buf, size_t count, Node *node) {
    /*
    * --------------------------------------------------------------------------------------
    *| msgType (1) | IV (16) | length (2) | NodeId (2) | Random (4) | Data (....) | CRC (4) |
    * --------------------------------------------------------------------------------------
    */

    uint8_t msgType_idx = 0;
    uint8_t iv_idx =      1;
    uint8_t length_idx =  iv_idx     + IV_LENGTH;
    uint8_t nodeId_idx =  length_idx + sizeof (int16_t);
    uint8_t nonce_idx =   nodeId_idx + sizeof (int16_t);
    uint8_t data_idx =    nonce_idx  + RANDOM_LENGTH;
    uint8_t crc_idx =     count      - CRC_LENGTH;

    uint8_t *iv;
    uint32_t crc;

    Crypto.decryptBuffer (&buf[length_idx], &buf[length_idx], count - length_idx, &buf[iv_idx], IV_LENGTH, node->getEncriptionKey (), KEY_LENGTH);
    DEBUG_VERBOSE ("Decripted data message: %s", printHexBuffer (buf, count));

    memcpy (&crc, &buf[crc_idx], CRC_LENGTH);

    if (!checkCRC (buf, count - 4, &crc)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    return true;

}


void manageMessage (uint8_t* mac, uint8_t* buf, uint8_t count/*, void* cbarg*/) {
    Node *node;

    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	DEBUG_VERBOSE ("Received data: %s", printHexBuffer (buf, count));

    if (count <= 1) {
        DEBUG_WARN ("Empty message");
        return;
    }

    node = nodelist.getNewNode (mac);

	flashBlue = true;

    int espNowError;

	switch (buf[0]) {
	case CLIENT_HELLO:
        // TODO: Do no accept new Client Hello if registration is on process on any node
        // TODO: Check message length
        DEBUG_INFO (" <------- CLIENT HELLO");
        espNowError = esp_now_add_peer (mac, ESP_NOW_ROLE_CONTROLLER, 0, NULL, 0);
        if (espNowError == 0) {
                if (processClientHello (mac, buf, count, node)) {
                node->setStatus (WAIT_FOR_KEY_EXCH_FINISHED);
                delay (1000);
                if (serverHello (myPublicKey, node)) {
                    DEBUG_INFO ("Server Hello sent");
                } else {
                    DEBUG_INFO ("Error sending Server Hello");
                }

            } else {
                node->reset ();
                DEBUG_ERROR ("Error processing client hello");
            }
        } else {
            DEBUG_ERROR ("Error adding peer %d", espNowError);
        }
        esp_now_del_peer (mac);
        break;
    case KEY_EXCHANGE_FINISHED:
        // TODO: Check message length
        // TODO: Check that ongoing registration belongs to this mac address
        if (node->getStatus() == WAIT_FOR_KEY_EXCH_FINISHED) {
            DEBUG_INFO (" <------- KEY EXCHANGE FINISHED");
            espNowError = esp_now_add_peer (mac, ESP_NOW_ROLE_CONTROLLER, 0, NULL, 0);
            if (espNowError == 0) {
                if (processKeyExchangeFinished (mac, buf, count, node)) {
                    if (cipherFinished (node)) {
                        node->setStatus (REGISTERED);
                        nodelist.printToSerial (&DEBUG_ESP_PORT);
                    } else {
                        node->reset ();
                    }
                } else {
                    node->reset ();
                }
            }
            esp_now_del_peer (mac);
        } else {
            DEBUG_INFO (" <------- unsolicited KEY EXCHANGE FINISHED");
        }
        break;
    case SENSOR_DATA:
        DEBUG_INFO (" <------- DATA");
        if (node->getStatus () == REGISTERED) {
            //espNowError = esp_now_add_peer (mac, ESP_NOW_ROLE_CONTROLLER, 0, NULL, 0);
            if (processDataMessage (mac, buf, count, node)) {
                DEBUG_INFO ("Data OK");
            } else {
                DEBUG_INFO ("Data not OK");
            }

        }
	}
}

void initEspNow () {
	/*bool ok = WifiEspNow.begin ();
	if (!ok) {
		Serial.println ("WifiEspNow.begin() failed");
		ESP.restart ();
	}
	WifiEspNow.onReceive (manageMessage, NULL);*/
    if (esp_now_init () != 0) {
        Serial.println ("Protocolo ESP-NOW no inicializado...");
        ESP.restart ();
        delay (1);
    }
    esp_now_set_self_role (ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb (manageMessage);
}

void setup () {
	Serial.begin (115200); Serial.println (); Serial.println ();

	pinMode (BLUE_LED, OUTPUT);
	digitalWrite (BLUE_LED, HIGH);
	pinMode (RED_LED, OUTPUT);
	digitalWrite (RED_LED, HIGH);

	initWiFi ();
	initEspNow ();

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

	if (flashRed) {
		redOntime = millis ();
		digitalWrite (RED_LED, LOW);
		flashRed = false;
	}

	if (!digitalRead (RED_LED) && millis () - redOntime > LED_PERIOD) {
		digitalWrite (RED_LED, HIGH);
	}
}