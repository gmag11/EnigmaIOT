// Node.h

#ifndef _NODE_h
#define _NODE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

enum node_status {
    UNREGISTERED,
    INIT,
    WAIT_FOR_SERVER_HELLO,
    WAIT_FOR_KEY_EXCH_FINISHED,
    WAIT_FOR_CIPHER_FINISHED,
    WAIT_FOR_DOWNLINK,
    REGISTERED,
    SLEEP
};

typedef enum node_status status_t;

struct node_instance {
    uint8_t mac[6];
    uint16_t nodeId;
    uint8_t key[32];
    uint16_t lastMessageCounter;
    time_t keyValidFrom;
    time_t lastMessageTime;
    status_t status = UNREGISTERED;
    bool keyValid = false;
};

typedef struct node_instance node_t;

class Node {
public:
    Node ();
    Node (node_t nodeData);
    uint8_t *getMacAddress () {
        return mac;
    }
    uint16_t getNodeId () {
        return nodeId;
    }
    void setNodeId (uint16_t nodeId) {
        this->nodeId = nodeId;
    }
    uint8_t *getEncriptionKey () {
        return key;
    }
    void setEncryptionKey (const uint8_t* key);
    time_t getKeyValidFrom () {
        return keyValidFrom;
    }
    void setKeyValidFrom (time_t keyValidFrom) {
        this->keyValidFrom = keyValidFrom;
    }
    time_t getLastMessageTime () {
        return lastMessageTime;
    }
    void setLastMessageTime () {
        lastMessageTime = millis ();
    }
    uint16_t getLastMessageCounter () {
        return lastMessageCounter;
    }
    void setLastMessageCounter (uint16_t counter) {
        lastMessageCounter = counter;
    }
    void setMacAddress (uint8_t *mac);

    bool isKeyValid () {
        return keyValid;
    }
    void setKeyValid (bool status) {
        keyValid = status;
    }
    bool isRegistered () {
        return status == REGISTERED;
    }
    status_t getStatus () {
        return status;
    }
    void setStatus (status_t status) {
        this->status = status;
    }
    node_t getNodeData ();

    void printToSerial (Stream *port);

    void reset ();

protected:
#define KEYLENGTH 32
    uint8_t mac[6];
    uint16_t nodeId;
    uint8_t key[KEYLENGTH];
    uint16_t lastMessageCounter;
    time_t keyValidFrom;
    bool keyValid = false;
    status_t status = UNREGISTERED;
    time_t lastMessageTime;

    void setMacAddress (const uint8_t *macAddress);

};

#endif

