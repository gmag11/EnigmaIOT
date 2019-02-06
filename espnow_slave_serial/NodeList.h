// NodeList.h

#ifndef _NODELIST_h
#define _NODELIST_h

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
    unsigned int lastMessageCounter;
    time_t lastMessageTime;
    status_t status;
    //bool registered = false;
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
    /*void setNodeId (uint16_t nodeId) {
        this->nodeId = nodeId;
    }*/
    uint8_t *getEncriptionKey () {
        return key;
    }
    void setEncryptionKey (const uint8_t* key);
    time_t getLastMessageTime () {
        return lastMessageTime;
    }
    time_t getLastMessageTime (time_t lastMessageTime) {
        lastMessageTime = lastMessageTime;
    }
    void setLastMessageTime (time_t lastMessageTime) {
        this->lastMessageTime = lastMessageTime;
    }
    unsigned int getLastMessageCounter(){
        return lastMessageCounter;
    }
    void setLastMessageCounter (unsigned int counter) {
        lastMessageCounter = counter;
    }

    bool isKeyValid () {
        return keyValid;
    }
    void setKeyValid (bool status) {
        keyValid = status;
    }
    bool isRegistered () {
        return status == REGISTERED;
    }
    /*void setRegistered (boolean reg) {
        registered = reg;
    }*/
    status_t getStatus () {
        return status;
    }
    void setStatus (status_t status) {
        this->status = status;
    }
    node_t getNodeData ();

    String toString ();

protected:
#define KEYLENGTH 32
    uint8_t mac[6];
    uint16_t nodeId;
    uint8_t key[KEYLENGTH];
    unsigned int lastMessageCounter;
    time_t lastMessageTime;
    bool keyValid = false;
    //bool registered = false; 
    status_t status;

    void setMacAddress (const uint8_t *macAddress) {
        if (macAddress) {
            memcpy (mac, macAddress, 6);
        }
    }

    friend class NodeList;
};



class NodeList {
#define NUM_NODES 20
public:
    NodeList ();

    Node *getNodeFromID (uint16_t nodeId);

    Node *getNodeFromMAC (const uint8_t* mac);
    
    Node *findEmptyNode ();
    
    uint16_t countActiveNodes ();
    
    bool unregisterNode (uint16_t nodeId);
    
    bool unregisterNode (const uint8_t* mac);
    
    bool unregisterNode (Node *node);

    Node *getNextActiveNode (uint16_t nodeId);

    Node *getNextActiveNode (Node node);

    Node *getNewNode (const uint8_t* mac);

protected:
    Node nodes[NUM_NODES];

};


#endif

