/**
  * @file NodeList.h
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief EnigmaIoT sensor node management structures
  */

#ifndef _NODELIST_h
#define _NODELIST_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

/**
  * @brief State definition for nodes
  */
enum node_status {
    UNREGISTERED, /**< Node is not registered. This is te initial state of every node */
    INIT, /**< Node is starting registration by ClientHello message */
    WAIT_FOR_SERVER_HELLO, /**< Node sent ClientHello message, now it is waiting for ServerHello */
    WAIT_FOR_KEY_EXCH_FINISHED, /**< Gateway sent ServerHello message, now it is waiting for KeyExchangeFinished from Node*/
    WAIT_FOR_CIPHER_FINISHED, /**< Node sent KeyExchangeFinished message, now it is waiting for CipherFinished */
    WAIT_FOR_DOWNLINK, /**< Node sent a data message, now it is waiting for downlink data */
    REGISTERED, /**< Node is registered and its key is valid */
    SLEEP /**< Node is in sleep mode */
};

typedef enum node_status status_t; ///< @brief Node state

/**
  * @brief Struct that define node fields. Used for long term storage needs
  */
struct node_instance {
    uint8_t mac[6]; ///< @brief Node address
    uint16_t nodeId; ///< @brief Node identifier asigned by gateway
    uint8_t key[32]; ///< @brief Shared key
    uint16_t lastMessageCounter; ///< @brief Last message counter state for specific Node
    time_t keyValidFrom; ///< @brief Last time that Node and Gateway agreed a key
    time_t lastMessageTime; ///< @brief Last time a message was received by Node
    status_t status = UNREGISTERED; ///< @brief Node state
    bool keyValid = false; ///< @brief Node shared key valid
    bool sleepyNode = true; ///< @brief Node sleepy definition
};

typedef struct node_instance node_t;

/**
  * @brief Class definition for a single sensor Node
  */
class Node {
public:
    /**
      * @brief Plain constructor
      * @return Returns a new unregistered Node instance
      */
    Node ();

    /**
      * @brief Constructor that initializes data from another Node data
      * @param nodeData `node_instance` struct that contains initalization values for new Node
      * @return Returns a new Node instance with same data as given `node_instance` struct
      */
    Node (node_t nodeData);

    /**
      * @brief Gets address from Node
      * @return Returns a pointer to Node address
      */
    uint8_t *getMacAddress () {
        return mac;
    }

    /**
      * @brief Gets Node identifier
      * @return Returns Node identifier
      */
    uint16_t getNodeId () {
        return nodeId;
    }

    /**
      * @brief Sets a new Node identifier
      * @param nodeId New nodeId value
      */
    void setNodeId (uint16_t nodeId) {
        this->nodeId = nodeId;
    }

    /**
      * @brief Gets Node encryption key
      * @return Returns a pointer to Node encryption key
      */
    uint8_t *getEncriptionKey () {
        return key;
    }

    /**
      * @brief Sets encryption key
      * @param key New key
      */
    void setEncryptionKey (const uint8_t* key);

    /**
      * @brief Gets last time that key was agreed with gateway
      * @return Time in milliseconds of last key agreement
      */
    time_t getKeyValidFrom () {
        return keyValidFrom;
    }

    /**
      * @brief Sets time when key was agreed with gateway
      * @param keyValidFrom Time on key agreement
      */
    void setKeyValidFrom (time_t keyValidFrom) {
        this->keyValidFrom = keyValidFrom;
    }

    /**
      * @brief Gets last time that node sent a message
      * @return Time in milliseconds of last received node message
      */
    time_t getLastMessageTime () {
        return lastMessageTime;
    }

    /**
      * @brief Sets current moment as last node message time
      */
    void setLastMessageTime () {
        lastMessageTime = millis ();
    }
    uint16_t getLastMessageCounter () {
        return lastMessageCounter;
    }
    void setLastMessageCounter (uint16_t counter) {
        lastMessageCounter = counter;
    }

    void setMacAddress (uint8_t *macAddress) {
        if (macAddress) {
            memcpy (mac, macAddress, 6);
        }
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
    status_t getStatus () {
        return status;
    }
    void setStatus (status_t status) {
        this->status = status;
    }
    node_t getNodeData ();

    void printToSerial (Stream *port);

    void reset ();

    void setSleepy (bool sleepy) {
        sleepyNode = sleepy;
    }

    bool getSleepy () {
        return sleepyNode;
    }

protected:
#define KEYLENGTH 32
    uint8_t mac[6]; ///< @brief Node address
    uint8_t key[KEYLENGTH]; ///< @brief Shared key
    uint16_t lastMessageCounter; ///< @brief Last message counter state for specific Node
    uint16_t nodeId; ///< @brief Node identifier asigned by gateway
    timer_t keyValidFrom; ///< @brief Last time that Node and Gateway agreed a key
    bool keyValid; ///< @brief Node shared key valid
    status_t status = UNREGISTERED;
    timer_t lastMessageTime; ///< @brief Node state
    bool sleepyNode = true; ///< @brief Node sleepy definition

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

    void printToSerial (Stream *port);

protected:
    Node nodes[NUM_NODES]; ///< @brief Static Node array that holds maximum number of supported nodes 

};


#endif

