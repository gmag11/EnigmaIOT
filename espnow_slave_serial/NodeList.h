// NodeList.h

#ifndef _NODELIST_h
#define _NODELIST_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

struct node_instance {
    uint8_t mac[6];
    uint16_t nodeId;
    uint8_t key[32];
    time_t lastMessage;
    //status_t status;
    bool keyValid = false;
};

typedef struct node_instance node_t;

class Node {
public:
    Node (node_t nodeData);
    uint8_t *getMacAddress () {
        return mac;
    }
    void setMacAddress (uint8_t *macAddress) {
        if (macAddress) {
            memcpy (mac, macAddress, 6);
        }
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
    void setEncryptionKey (uint8_t* key);
    time_t getLastMessageTime () {
        return lastMessage;
    }
    void setLastMessageTime (time_t lastMessageTime) {
        lastMessage = lastMessageTime;
    }
    bool isKeyValid () {
        return keyValid;
    }
    void setKeyValid (bool status) {
        keyValid = status;
    }
    node_t getNodeData ();
protected:
#define KEYLENGTH 32
    uint8_t mac[6];
    uint16_t nodeId;
    uint8_t key[KEYLENGTH];
    time_t lastMessage;
    bool keyValid = false;

    friend class NodeList;
};



class NodeList {
#define NUM_NODES 20
    Node *getNode (uint16_t nodeId);

    Node *getNode (uint8_t* mac);
    
    Node *findEmptyNode ();
    
    uint16_t countActiveNodes ();
    
    bool deleteNode (uint16_t nodeId);
    
    bool deleteNode (uint8_t* mac);
    
    bool deleteNode (Node node);

    Node *getNextActiveNode (uint16_t nodeId);

    Node *getNextActiveNode (Node node);

protected:
    Node nodes[NUM_NODES];

    friend class Node;
};


#endif

