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

    /**
      * @brief Gets counter for last received message from node
      * @return Message counter
      */
    uint16_t getLastMessageCounter () {
        return lastMessageCounter;
    }

    /**
      * @brief Sets counter for last received message from node
      * @param counter Message counter
      */
    void setLastMessageCounter (uint16_t counter) {
        lastMessageCounter = counter;
    }

    /**
      * @brief Sets node address
      * @param macAddress Node address
      */
    void setMacAddress (uint8_t *macAddress) {
        if (macAddress) {
            memcpy (mac, macAddress, 6);
        }
    }

    /**
      * @brief Gets shared key validity for this node
      * @return `true` if node shared key is valid. `false` otherwise
      */
    bool isKeyValid () {
        return keyValid;
    }

    /**
      * @brief Sets shared key validity for this node
      * @param status node key validity
      */
    void setKeyValid (bool status) {
        keyValid = status;
    }

    /**
      * @brief Gets registration state of this node
      * @return `true` if node is registered on gateway. `false` otherwise
      */
    bool isRegistered () {
        return status == REGISTERED;
    }

    /**
      * @brief Gets status for finite state machine that represents node
      * @return Node status
      */
    status_t getStatus () {
        return status;
    }

    /**
      * @brief Sets status for finite state machine that represents node
      * @param status Node status
      */
    void setStatus (status_t status) {
        this->status = status;
    }

    /**
      * @brief Gets a struct that represents node object. May be used for node serialization
      * @return Node struct
      */
    node_t getNodeData ();

    /**
      * @brief Dumps node data to the given stream, Serial by default. This method may be used for debugging
      * @param port Stream to print data to
      */
    void printToSerial (Stream *port = &Serial);

    /**
      * @brief Resets all node fields to a default initial and not registered state
      */
    void reset ();

    /**
      * @brief Sets node working mode regarding battery saving strategy. If node is sleepy it will turn into deep sleep
      * after sending a message. In this case it will wait for a short while for a downlink message from gateway
      * @param sleepy `true` if node sleeps after sending a message and wait for downlink. `false` if downlink may happen in any moment
      */
    void setSleepy (bool sleepy) {
        sleepyNode = sleepy;
    }

    /**
      * @brief Gets node working mode regarding battery saving strategy. If node is sleepy it will turn into deep sleep
      * after sending a message. In this case it will wait for a short while for a downlink message from gateway
      * @return `true` if node sleeps after sending a message and wait for downlink. `false` if downlink may happen in any moment
      */
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

