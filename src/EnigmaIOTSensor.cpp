// 
// 

#include "EnigmaIOTSensor.h"

void EnigmaIOTSensorClass::setLed (uint8_t led, time_t onTime) {
    this->led = led;
    ledOnTime = onTime;
}

void EnigmaIOTSensorClass::begin (Comms_halClass *comm, uint8_t *gateway, bool useCounter, bool sleepy) {
    pinMode (led, OUTPUT);
    digitalWrite (led, HIGH);

    memcpy (this->gateway, gateway, comm->getAddressLength ());
    initWiFi ();
    this->comm = comm;
    comm->begin (gateway,channel);
    comm->onDataRcvd (rx_cb);
    comm->onDataSent (tx_cb);
    this->useCounter = useCounter;

    node.setSleepy (sleepy);

    if (ESP.rtcUserMemoryRead (0, (uint32_t*)&rtcmem_data, sizeof (rtcmem_data))) {
        DEBUG_VERBOSE ("Read RTCData: %s", printHexBuffer ((uint8_t *)&rtcmem_data, sizeof (rtcmem_data)));
    }
    if (!checkCRC ((uint8_t *)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t), &rtcmem_data.crc32)) {
        clientHello ();
    } else {
        node.setEncryptionKey (rtcmem_data.nodeKey);
        node.setKeyValid (true);
        node.setLastMessageCounter (rtcmem_data.lastMessageCounter);
        node.setLastMessageTime ();
        node.setStatus (REGISTERED);
        uint8_t macAddress[6];
        if (wifi_get_macaddr (0, macAddress)) {
            node.setMacAddress (macAddress);
        }
        node.setKeyValidFrom (millis ());
        node.setNodeId (rtcmem_data.nodeId);
    }

}

void EnigmaIOTSensorClass::handle () {
    static unsigned long blueOntime;

    if (led >= 0) {
        if (flashBlue) {
            blueOntime = millis ();
            digitalWrite (led, LOW);
            flashBlue = false;
        }

        if (!digitalRead (led) && millis () - blueOntime > ledOnTime) {
            digitalWrite (led, HIGH);
        }
    }

    if (sleepRequested && millis () - node.getLastMessageTime () > DOWNLINK_WAIT_TIME && node.isRegistered()) {
        DEBUG_VERBOSE ("Go to sleep for %u ms", sleepTime / 1000);
        ESP.deepSleepInstant (sleepTime, RF_NO_CAL);
    }


    static time_t lastRegistration;
    status_t status = node.getStatus ();
    if (status == WAIT_FOR_SERVER_HELLO || status == WAIT_FOR_CIPHER_FINISHED) {
        if (millis () - lastRegistration > (RECONNECTION_PERIOD*10)) {
            DEBUG_VERBOSE ("Current node status: %d", node.getStatus ());
            lastRegistration = millis ();
            node.reset ();
            clientHello ();
        }
    }


    if (node.getStatus()== UNREGISTERED) {
        if (millis () - lastRegistration > (RECONNECTION_PERIOD)) {
            DEBUG_VERBOSE ("Current node status: %d", node.getStatus ());
            lastRegistration = millis ();
            node.reset ();
            clientHello ();
        }
    }

}

void EnigmaIOTSensorClass::rx_cb (u8 *mac_addr, u8 *data, u8 len) {
    EnigmaIOTSensor.manageMessage (mac_addr, data, len);
}

void EnigmaIOTSensorClass::tx_cb (u8 *mac_addr, u8 status) {
    EnigmaIOTSensor.getStatus (mac_addr, status);
}

bool EnigmaIOTSensorClass::checkCRC (const uint8_t *buf, size_t count, uint32_t *crc) {
    uint32 recvdCRC;

    memcpy (&recvdCRC, crc, sizeof (uint32_t));
    //DEBUG_VERBOSE ("Received CRC32: 0x%08X", *crc32);
    uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X Length: %d", _crc, recvdCRC, count);
    return (_crc == recvdCRC);
}

bool EnigmaIOTSensorClass::clientHello () {
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

    Crypto.getDH1 ();
    node.setStatus (INIT);
    uint8_t macAddress[6];
    if (wifi_get_macaddr (0, macAddress)) {
        node.setMacAddress (macAddress);
    }
    uint8_t *key = Crypto.getPubDHKey ();

    if (!key) {
        return false;
    }

    clientHello_msg.msgType = CLIENT_HELLO; // Client hello message

    CryptModule::random (clientHello_msg.iv, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (clientHello_msg.iv, IV_LENGTH));

    for (int i = 0; i < KEY_LENGTH; i++) {
        clientHello_msg.publicKey[i] = key[i];
    }

    crc32 = CRC32::calculate ((uint8_t *)&clientHello_msg, CHMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    memcpy (&(clientHello_msg.crc), &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer ((uint8_t*)&clientHello_msg, CHMSG_LEN));

    node.setStatus (WAIT_FOR_SERVER_HELLO);

    DEBUG_INFO (" -------> CLIENT HELLO");

    return comm->send (gateway, (uint8_t*)&clientHello_msg, CHMSG_LEN) == 0;
}


bool EnigmaIOTSensorClass::processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count) {
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

    //uint8_t myPublicKey[KEY_LENGTH];
    uint32_t crc32;

    if (count < SHMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

    memcpy (&serverHello_msg, buf, count);

    memcpy (&crc32, &(serverHello_msg.crc), CRC_LENGTH);

    if (!checkCRC (buf, count - CRC_LENGTH, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    Crypto.getDH2 (serverHello_msg.publicKey);
    node.setEncryptionKey (serverHello_msg.publicKey);
    DEBUG_INFO ("Node key: %s", printHexBuffer (node.getEncriptionKey (), KEY_LENGTH));

    return true;
}

bool EnigmaIOTSensorClass::processCipherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count) {
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

    uint16_t nodeId;
    uint32_t crc32;

    if (count < CFMSG_LEN) {
        DEBUG_WARN ("Wrong message length --> Required: %d Received: %d", CFMSG_LEN, count);
        return false;
    }

    memcpy (&cipherFinished_msg, buf, CFMSG_LEN);

    Crypto.decryptBuffer (
        (uint8_t *)&(cipherFinished_msg.nodeId),
        (uint8_t *)&(cipherFinished_msg.nodeId),
        CFMSG_LEN - IV_LENGTH - 1,
        cipherFinished_msg.iv,
        IV_LENGTH,
        node.getEncriptionKey (),
        KEY_LENGTH
    );
    DEBUG_VERBOSE ("Decripted Cipher Finished message: %s", printHexBuffer ((uint8_t *)&cipherFinished_msg, CFMSG_LEN));

    memcpy (&crc32, &(cipherFinished_msg.crc), CRC_LENGTH);

    if (!checkCRC ((uint8_t *)&cipherFinished_msg, CFMSG_LEN - 4, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    memcpy (&nodeId, &cipherFinished_msg.nodeId, sizeof (uint16_t));
    node.setNodeId (nodeId);
    DEBUG_VERBOSE ("Node ID: %u", node.getNodeId ());
    return true;
}

bool EnigmaIOTSensorClass::keyExchangeFinished () {
    /*
    * -------------------------------------------------------------------------
    *| msgType (1) | IV (16) | random (31 bits) | SleepyNode (1 bit) | CRC (4) |
    * -------------------------------------------------------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t iv[IV_LENGTH];
        uint32_t random;
        uint32_t crc;
    } keyExchangeFinished_msg;

#define KEFMSG_LEN sizeof(keyExchangeFinished_msg) 

    uint32_t crc32;
    uint32_t random;

    keyExchangeFinished_msg.msgType = KEY_EXCHANGE_FINISHED;

    Crypto.random (keyExchangeFinished_msg.iv, IV_LENGTH);
    DEBUG_VERBOSE ("IV: %s", printHexBuffer (keyExchangeFinished_msg.iv, IV_LENGTH));

    random = Crypto.random ();

    if (node.getSleepy ()) {
        random = random | 0x00000001U; // Signal sleepy node
    }
    else {
        random = random & 0xFFFFFFFEU; // Signal always awake node
    }

    memcpy (&(keyExchangeFinished_msg.random), &random, RANDOM_LENGTH);

    crc32 = CRC32::calculate ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    memcpy (&(keyExchangeFinished_msg.crc), &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Key Exchange Finished message: %s", printHexBuffer ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN));

    Crypto.encryptBuffer ((uint8_t *)&(keyExchangeFinished_msg.random), (uint8_t *)&(keyExchangeFinished_msg.random), KEFMSG_LEN - IV_LENGTH - 1, keyExchangeFinished_msg.iv, IV_LENGTH, node.getEncriptionKey (), KEY_LENGTH);

    DEBUG_VERBOSE ("Encripted Key Exchange Finished message: %s", printHexBuffer ((uint8_t *)&(keyExchangeFinished_msg), KEFMSG_LEN));
    DEBUG_INFO (" -------> KEY_EXCHANGE_FINISHED");
    return comm->send (gateway, (uint8_t *)&(keyExchangeFinished_msg), KEFMSG_LEN) == 0;
}

bool EnigmaIOTSensorClass::sendData (const uint8_t *data, size_t len) {
    memcpy (dataMessageSent, data, len);
    dataMessageSentLength = len;

    if (node.getStatus () == REGISTERED && node.isKeyValid ()) {
        DEBUG_INFO ("Data sent: %s", printHexBuffer (data, len));
        flashBlue = true;
        return dataMessage ((uint8_t *)data, len);
    }
    return false;
}

void EnigmaIOTSensorClass::sleep (uint64_t time)
{
    DEBUG_VERBOSE ("Sleep programmed for %u ms", time/1000);
    sleepTime = time;
    sleepRequested = true;
}

bool EnigmaIOTSensorClass::dataMessage (const uint8_t *data, size_t len) {
    /*
    * --------------------------------------------------------------------------------------
    *| msgType (1) | IV (16) | length (2) | NodeId (2) | Counter (2) | Data (....) | CRC (4) |
    * --------------------------------------------------------------------------------------
    */

    uint8_t buffer[200];
    uint32_t crc32;
    uint32_t counter;
    uint16_t nodeId = node.getNodeId ();

    uint8_t *msgType_p = buffer;
    uint8_t *iv_p = buffer + 1;
    uint8_t *length_p = buffer + 1 + IV_LENGTH;
    uint8_t *nodeId_p = buffer + 1 + IV_LENGTH + sizeof (int16_t);
    uint8_t *counter_p = buffer + 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t);
    uint8_t *data_p = buffer + 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + sizeof (int16_t);

    if (!data) {
        return false;
    }

    *msgType_p = (uint8_t)SENSOR_DATA;

    CryptModule::random (iv_p, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (iv_p, IV_LENGTH));

    memcpy (nodeId_p, &nodeId, sizeof (uint16_t));

    if (useCounter) {
        counter = node.getLastMessageCounter() + 1;
        node.setLastMessageCounter (counter);
        rtcmem_data.lastMessageCounter = counter;
    }
    else {
        counter = Crypto.random ();
    }

    memcpy (counter_p, &counter, sizeof(uint16_t));

    memcpy (data_p, data, len);

    uint16_t packet_length = 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + sizeof (uint16_t) + len;

    memcpy (length_p, &packet_length, sizeof (uint16_t));

    crc32 = CRC32::calculate (buffer, packet_length);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    // int is little indian mode on ESP platform
    uint8_t *crc_p = (uint8_t*)(buffer + packet_length);

    memcpy (crc_p, &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Data message: %s", printHexBuffer (buffer, packet_length + CRC_LENGTH));

    uint8_t *crypt_buf = buffer + 1 + IV_LENGTH;

    size_t cryptLen = packet_length + CRC_LENGTH - 1 - IV_LENGTH;

    Crypto.encryptBuffer (crypt_buf, crypt_buf, cryptLen, iv_p, IV_LENGTH, node.getEncriptionKey (), KEY_LENGTH);

    DEBUG_VERBOSE ("Encrypted data message: %s", printHexBuffer (buffer, packet_length + CRC_LENGTH));

    DEBUG_INFO (" -------> DATA");

    if (useCounter) {
        rtcmem_data.crc32 = CRC32::calculate ((uint8_t *)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
        if (ESP.rtcUserMemoryWrite (0, (uint32_t*)&rtcmem_data, sizeof (rtcmem_data))) {
            DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t *)&rtcmem_data, sizeof (rtcmem_data)));
        }
    }

    return comm->send (gateway, buffer, packet_length + CRC_LENGTH) == 0;
}

sensorInvalidateReason_t EnigmaIOTSensorClass::processInvalidateKey (const uint8_t mac[6], const uint8_t* buf, size_t count) {
#define IKMSG_LEN 2
    if (buf && count < IKMSG_LEN) {
        return UNKNOWN_ERROR;
    }
    DEBUG_VERBOSE ("Invalidate key request. Reason: %u", buf[1]);
    uint8_t reason = buf[1];
    return (sensorInvalidateReason_t)reason;
}

void EnigmaIOTSensorClass::manageMessage (const uint8_t *mac, const uint8_t* buf, uint8_t count) {
    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s", printHexBuffer (const_cast<uint8_t *>(buf), count));
    flashBlue = true;

    if (count <= 1) {
        return;
    }

    switch (buf[0]) {
    case SERVER_HELLO:
        DEBUG_INFO (" <------- SERVER HELLO");
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
        if (node.getStatus () == WAIT_FOR_CIPHER_FINISHED) {
            DEBUG_INFO (" <------- CIPHER_FINISHED");
            if (processCipherFinished (mac, buf, count)) {
                // mark node as registered
                node.setKeyValid (true);
                node.setKeyValidFrom (millis ());
                node.setLastMessageCounter (0);
                node.setStatus (REGISTERED);
                
                memcpy (rtcmem_data.nodeKey, node.getEncriptionKey (), KEY_LENGTH);
                rtcmem_data.lastMessageCounter = 0;
                rtcmem_data.nodeId = node.getNodeId ();
                rtcmem_data.crc32 = CRC32::calculate ((uint8_t *)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
                if (ESP.rtcUserMemoryWrite (0, (uint32_t*)&rtcmem_data, sizeof (rtcmem_data))) {
                    DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t *)&rtcmem_data, sizeof (rtcmem_data)));
                }

#if DEBUG_LEVEL >= INFO
                node.printToSerial (&DEBUG_ESP_PORT);
#endif
                if (notifyConnection) {
                    notifyConnection ();
                }
                // Resend last message in case of it is still pending to be sent.
                // If key expired it was successfully sent before so retransmission is not needed 
                if (invalidateReason < KEY_EXPIRED && dataMessageSentLength > 0) {
                    if (node.getStatus () == REGISTERED && node.isKeyValid ()) {
                        DEBUG_INFO ("Data sent: %s", printHexBuffer (dataMessageSent, dataMessageSentLength));
                        dataMessage ((uint8_t *)dataMessageSent, dataMessageSentLength);
                        flashBlue = true;
                    }
                }
                // TODO: Store node data on EEPROM, SPIFFS or RTCMEM
                
            } else {
                node.reset ();
            }
        } else {
            node.reset ();
        }
        break;
    case INVALIDATE_KEY:
        DEBUG_INFO (" <------- INVALIDATE KEY");
        invalidateReason = processInvalidateKey (mac, buf, count);
        node.reset ();
        if (notifyDisconnection) {
            notifyDisconnection ();
        }
        //clientHello ();
    }
}

void EnigmaIOTSensorClass::getStatus (u8 *mac_addr, u8 status) {
    DEBUG_VERBOSE ("SENDStatus %s", status==0?"OK":"ERROR");
}


EnigmaIOTSensorClass EnigmaIOTSensor;

