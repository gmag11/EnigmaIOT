// 
// 
// 

#include "SecureSensorGateway.h"


void SecureSensorGatewayClass::setTxLed (uint8_t led, time_t onTime) {
    this->txled = led;
    txLedOnTime = onTime;
    pinMode (txled, OUTPUT);
    digitalWrite (txled, HIGH);
}

void SecureSensorGatewayClass::setRxLed (uint8_t led, time_t onTime) {
    this->rxled = led;
    rxLedOnTime = onTime;
    pinMode (rxled, OUTPUT);
    digitalWrite (rxled, HIGH);
}

void SecureSensorGatewayClass::begin (Comms_halClass *comm, bool useDataCounter) {
    this->comm = comm;
    this->useCounter = useDataCounter;
    comm->begin (NULL, 0, COMM_GATEWAY);
    comm->onDataRcvd (rx_cb);
    comm->onDataSent (tx_cb);
}

void SecureSensorGatewayClass::rx_cb (u8 *mac_addr, u8 *data, u8 len) {
    SecureSensorGateway.manageMessage (mac_addr, data, len);
}

void SecureSensorGatewayClass::tx_cb (u8 *mac_addr, u8 status) {
    SecureSensorGateway.getStatus (mac_addr, status);
}

void SecureSensorGatewayClass::getStatus (u8 *mac_addr, u8 status) {
    DEBUG_VERBOSE ("SENDStatus %s", status == 0 ? "OK" : "ERROR");
}

void SecureSensorGatewayClass::handle () {
    static unsigned long rxOntime;
    static unsigned long txOntime;

    if (flashRx) {
        rxOntime = millis ();
        digitalWrite (rxled, LOW);
        flashRx = false;
    }

    if (!digitalRead (rxled) && millis () - rxOntime > rxLedOnTime) {
        digitalWrite (rxled, HIGH);
    }

    if (flashTx) {
        txOntime = millis ();
        digitalWrite (txled, LOW);
        flashTx = false;
    }

    if (!digitalRead (txled) && millis () - txOntime > txLedOnTime) {
        digitalWrite (txled, HIGH);
    }

#define MAX_INACTIVITY 86400000U
    // Clean up dead nodes
    for (int i = 0; i < NUM_NODES; i++) {
        Node *node = nodelist.getNodeFromID (i);
        if (node->isRegistered () && millis () - node->getLastMessageTime () > MAX_INACTIVITY) {
            // TODO. Trigger node expired event
            node->reset ();
        }
    }

}

void SecureSensorGatewayClass::manageMessage (const uint8_t* mac, const uint8_t* buf, uint8_t count) {
    Node *node;

    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s", printHexBuffer (buf, count));

    if (count <= 1) {
        DEBUG_WARN ("Empty message");
        return;
    }

    node = nodelist.getNewNode (mac);

    flashRx = true;

    int espNowError = 0;

    switch (buf[0]) {
    case CLIENT_HELLO:
        // TODO: Do no accept new Client Hello if registration is on process on any node??
        DEBUG_INFO (" <------- CLIENT HELLO");
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
                invalidateKey (node, WRONG_CLIENT_HELLO);
                node->reset ();
                DEBUG_ERROR ("Error processing client hello");
            }
        } else {
            DEBUG_ERROR ("Error adding peer %d", espNowError);
        }
        break;
    case KEY_EXCHANGE_FINISHED:
        if (node->getStatus () == WAIT_FOR_KEY_EXCH_FINISHED) {
            DEBUG_INFO (" <------- KEY EXCHANGE FINISHED");
            if (espNowError == 0) {
                if (processKeyExchangeFinished (mac, buf, count, node)) {
                    if (cipherFinished (node)) {
                        node->setStatus (REGISTERED);
                        node->setKeyValidFrom (millis ());
                        node->setLastMessageCounter (0);
                        node->setLastMessageTime ();
                        // TODO. Trigger new node event
#if DEBUG_LEVEL >= INFO
                        nodelist.printToSerial (&DEBUG_ESP_PORT);
#endif
                    } else {
                        node->reset ();
                    }
                } else {
                    invalidateKey (node, WRONG_EXCHANGE_FINISHED);
                    node->reset ();
                }
            }
        } else {
            DEBUG_INFO (" <------- unsolicited KEY EXCHANGE FINISHED");
        }
        break;
    case SENSOR_DATA:
        DEBUG_INFO (" <------- DATA");
        if (node->getStatus () == REGISTERED) {
            if (processDataMessage (mac, buf, count, node)) {
                node->setLastMessageTime ();
                DEBUG_INFO ("Data OK");
                DEBUG_VERBOSE ("Key valid from %u ms", millis () - node->getKeyValidFrom ());
                if (millis () - node->getKeyValidFrom () > MAX_KEY_VALIDITY) {
                    invalidateKey (node, KEY_EXPIRED);
                }
            } else {
                invalidateKey (node, WRONG_DATA);
                DEBUG_INFO ("Data not OK");
            }

        } else {
            invalidateKey (node, UNREGISTERED_NODE);
        }
    }
}

bool SecureSensorGatewayClass::processDataMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node) {
    /*
    * ---------------------------------------------------------------------------------------
    *| msgType (1) | IV (16) | length (2) | NodeId (2) | Counter (2) | Data (....) | CRC (4) |
    * ---------------------------------------------------------------------------------------
    */

    uint8_t msgType_idx = 0;
    uint8_t iv_idx = 1;
    uint8_t length_idx = iv_idx + IV_LENGTH;
    uint8_t nodeId_idx = length_idx + sizeof (int16_t);
    uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
    uint8_t data_idx = counter_idx + sizeof (int16_t);
    uint8_t crc_idx = count - CRC_LENGTH;

    uint8_t *iv;
    uint32_t crc32;
    uint16_t counter;
    size_t lostMessages;

    Crypto.decryptBuffer (
        const_cast<uint8_t *>(&buf[length_idx]),
        const_cast<uint8_t *>(&buf[length_idx]),
        count - length_idx,
        const_cast<uint8_t *>(&buf[iv_idx]),
        IV_LENGTH,
        node->getEncriptionKey (),
        KEY_LENGTH
    );
    DEBUG_VERBOSE ("Decripted data message: %s", printHexBuffer (buf, count));

    memcpy (&counter, &buf[counter_idx], sizeof(uint16_t));
    if (useCounter) {
        if (counter > node->getLastMessageCounter()) {
            lostMessages = counter - node->getLastMessageCounter () - 1;
            node->setLastMessageCounter (counter);
        } else {
            return false;
        }
    }

    memcpy (&crc32, &buf[crc_idx], CRC_LENGTH);

    if (!checkCRC (buf, count - 4, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    if (notifyData) {
        notifyData (mac, &buf[data_idx], crc_idx - data_idx, lostMessages);
    }

    return true;

}

bool SecureSensorGatewayClass::processKeyExchangeFinished (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node) {
    /*
    * ----------------------------------------------
    *| msgType (1) | IV (16) | random (4) | CRC (4) |
    * ----------------------------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t iv[IV_LENGTH];
        uint32_t random;
        uint32_t crc;
    } keyExchangeFinished_msg;

#define KEFMSG_LEN sizeof(keyExchangeFinished_msg)

    uint32_t crc32;

    if (count < KEFMSG_LEN) {
        DEBUG_WARN ("Wrong message");
        return false;
    }

    memcpy (&keyExchangeFinished_msg, buf, KEFMSG_LEN);

    Crypto.decryptBuffer (
        (uint8_t *)&(keyExchangeFinished_msg.random),
        (uint8_t *)&(keyExchangeFinished_msg.random),
        RANDOM_LENGTH + CRC_LENGTH,
        keyExchangeFinished_msg.iv,
        IV_LENGTH,
        node->getEncriptionKey (),
        KEY_LENGTH
    );

    DEBUG_VERBOSE ("Decripted Key Exchange Finished message: %s", printHexBuffer ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN));

    memcpy (&crc32, &(keyExchangeFinished_msg.crc), CRC_LENGTH);

    if (!checkCRC ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN - CRC_LENGTH, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    return true;

}

bool SecureSensorGatewayClass::cipherFinished (Node *node) {
    /*
    * -----------------------------------------------------------
    *| msgType (1) | IV (16) | nodeId (2) | random (4) | CRC (4) |
    * -----------------------------------------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t iv[IV_LENGTH];
        uint16_t nodeId;
        uint32_t random;
        uint32_t crc;
    } cipherFinished_msg;

#define CFMSG_LEN sizeof(cipherFinished_msg)

    uint32_t crc32;
    uint32_t random;

    cipherFinished_msg.msgType = CYPHER_FINISHED; // Server hello message

    CryptModule::random (cipherFinished_msg.iv, IV_LENGTH);
    DEBUG_VERBOSE ("IV: %s", printHexBuffer (cipherFinished_msg.iv, IV_LENGTH));

    uint16_t nodeId = node->getNodeId ();
    memcpy (&(cipherFinished_msg.nodeId), &nodeId, sizeof (uint16_t));

    random = Crypto.random ();

    memcpy (&(cipherFinished_msg.random), &random, RANDOM_LENGTH);

    crc32 = CRC32::calculate ((uint8_t *)&cipherFinished_msg, CFMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    memcpy (&(cipherFinished_msg.crc), (uint8_t *)&crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Cipher Finished message: %s", printHexBuffer ((uint8_t *)&cipherFinished_msg, CFMSG_LEN));

    Crypto.encryptBuffer (
        (uint8_t *)&(cipherFinished_msg.nodeId), 
        (uint8_t *)&(cipherFinished_msg.nodeId), 
        2 + RANDOM_LENGTH + CRC_LENGTH,
        cipherFinished_msg.iv, 
        IV_LENGTH, 
        node->getEncriptionKey (), 
        KEY_LENGTH
    );

    DEBUG_VERBOSE ("Encripted Cipher Finished message: %s", printHexBuffer ((uint8_t *)&cipherFinished_msg, CFMSG_LEN));

    flashTx = true;
    DEBUG_INFO (" -------> CYPHER_FINISHED");
    return comm->send (node->getMacAddress (), (uint8_t *)&cipherFinished_msg, CFMSG_LEN) == 0;
}

bool  SecureSensorGatewayClass::invalidateKey (Node *node, uint8_t reason) {
    /*
    * --------------------------
    *| msgType (1) | reason (1) |
    * --------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t reason;
        //uint32_t crc;
    } invalidateKey_msg;

#define IKMSG_LEN sizeof(invalidateKey_msg)

    invalidateKey_msg.msgType = INVALIDATE_KEY; // Server hello message

    invalidateKey_msg.reason = reason;

    //uint32_t crc = CRC32::calculate (buffer, 2);
    //DEBUG_VERBOSE ("CRC32 = 0x%08X", crc);
    //memcpy (&invalidateKey_msg.crc, (uint8_t *)&crc, CRC_LENGTH);
    DEBUG_VERBOSE ("Invalidate Key message: %s", printHexBuffer ((uint8_t *)&invalidateKey_msg, IKMSG_LEN));
    DEBUG_INFO (" -------> INVALIDATE_KEY");
    return comm->send (node->getMacAddress (), (uint8_t *)&invalidateKey_msg, IKMSG_LEN) == 0;
}

bool SecureSensorGatewayClass::processClientHello (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node) {
    /*
    * -------------------------------------------------------
    *| msgType (1) | random (16) | DH Kmaster (32) | CRC (4) |
    * -------------------------------------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t iv[IV_LENGTH];
        uint8_t publicKey[KEY_LENGTH];
        uint32_t crc;
    } clientHello_msg;

#define CHMSG_LEN sizeof(clientHello_msg)

    uint32_t crc32;

    if (count < CHMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

    memcpy (&clientHello_msg, buf, count);

    memcpy (&crc32, &(clientHello_msg.crc), sizeof (uint32_t));

    if (!checkCRC ((uint8_t*)&clientHello_msg, CHMSG_LEN - CRC_LENGTH, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    node->setEncryptionKey (clientHello_msg.publicKey);

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


bool SecureSensorGatewayClass::serverHello (const uint8_t *key, Node *node) {
    /*
    * ------------------------------------------------------
    *| msgType (1) | random (16) | DH Kslave (32) | CRC (4) |
    * ------------------------------------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t iv[IV_LENGTH];
        uint8_t publicKey[KEY_LENGTH];
        uint32_t crc;
    } serverHello_msg;

#define SHMSG_LEN sizeof(serverHello_msg)

    uint32_t crc32;

    if (!key) {
        DEBUG_ERROR ("NULL key");
        return false;
    }

    serverHello_msg.msgType = SERVER_HELLO; // Server hello message

    CryptModule::random (serverHello_msg.iv, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (serverHello_msg.iv, IV_LENGTH));

    for (int i = 0; i < KEY_LENGTH; i++) {
        serverHello_msg.publicKey[i] = key[i];
    }

    crc32 = CRC32::calculate ((uint8_t *)&serverHello_msg, SHMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    memcpy (&(serverHello_msg.crc), &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Server Hello message: %s", printHexBuffer ((uint8_t *)&serverHello_msg, SHMSG_LEN));
    flashTx = true;

#ifdef DEBUG_ESP_PORT
    char mac[18];
    mac2str (node->getMacAddress (), mac);
#endif
    DEBUG_INFO (" -------> SERVER_HELLO");
    if (comm->send (node->getMacAddress (), (uint8_t *)&serverHello_msg, SHMSG_LEN) == 0) {
        DEBUG_INFO ("Server Hello message sent to %s", mac);
        return true;
    } else {
        nodelist.unregisterNode (node);
        DEBUG_ERROR ("Error sending Server Hello message to %s", mac);
        return false;
    }
}

bool SecureSensorGatewayClass::checkCRC (const uint8_t *buf, size_t count, const uint32_t *crc) {
    uint32_t recvdCRC;

    memcpy (&recvdCRC, crc, sizeof (uint32_t)); // Use of memcpy is a must to ensure code does not try to read non memory aligned int
    uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X %s", _crc, recvdCRC, (_crc == recvdCRC) ? "OK" : "FAIL");
    return (_crc == recvdCRC);
}


SecureSensorGatewayClass SecureSensorGateway;

