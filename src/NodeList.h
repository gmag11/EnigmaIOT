/**
  * @file NodeList.h
  * @version 0.9.3
  * @date 14/07/2020
  * @author German Martin
  * @brief EnigmaIoT sensor node management structures
  */

#ifndef _NODELIST_h
#define _NODELIST_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include "EnigmaIoTconfig.h"
#include "Filter.h"

/**
  * @brief State definition for nodes
  */
enum node_status {
    UNREGISTERED, /**< Node is not registered. This is te initial state of every node */
    INIT, /**< Node is starting registration by ClientHello message */
    WAIT_FOR_SERVER_HELLO, /**< Node sent ClientHello message, now it is waiting for ServerHello */
    WAIT_FOR_DOWNLINK, /**< Node sent a data message, now it is waiting for downlink data */
    REGISTERED, /**< Node is registered and its key is valid */
    SLEEP /**< Node is in sleep mode */
};

typedef enum {
    NAME_OK = 0,
    ALREADY_USED = -1,
    EMPTY_NAME = -2,
    TOO_LONG = -3
}nodeNameStatus_t;

typedef enum node_status status_t; ///< @brief Node state

typedef enum control_message_type {
	VERSION = 0x01,
	VERSION_ANS = 0x81,
	SLEEP_GET = 0x02,
	SLEEP_SET = 0x03,
	SLEEP_ANS = 0x82,
	IDENTIFY = 0x04,
	RESET = 0x05,
	RESET_ANS = 0x85,
	RSSI_GET = 0x06,
	RSSI_ANS = 0x86,
    NAME_GET = 0x07,
    NAME_ANS = 0x08,
    NAME_SET = 0x87,
	OTA = 0xEF,
	OTA_ANS = 0xFF,
	USERDATA_GET = 0x00,
	USERDATA_SET = 0x20,
    INVALID = 0xF0
	//USERDATA_ANS = 0x90
} control_message_type_t;

typedef enum ota_status {
	OTA_STARTED = 0,
	OTA_START_ERROR = 1,
	OTA_CHECK_OK = 2,
	OTA_CHECK_FAIL = 3,
	OTA_OUT_OF_SEQUENCE =4 ,
	OTA_TIMEOUT = 5,
	OTA_FINISHED = 6
} ota_status_t;

/**
  * @brief Struct that define node fields. Used for long term storage needs
  */
struct node_instance {
    uint8_t mac[ENIGMAIOT_ADDR_LEN]; ///< @brief Node address
    uint16_t nodeId; ///< @brief Node identifier asigned by gateway
    uint8_t key[32]; ///< @brief Shared key
    uint16_t lastMessageCounter; ///< @brief Last message counter state for specific Node
    uint16_t lastControlCounter; ///< @brief Last control message counter state for specific Node
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
      * @brief Gets Node name
      * @return Returns Node name
      */
    char* getNodeName () {
        if (strlen (nodeName)) {
            return nodeName;
        } else {
            return NULL;
        }
    }

    /**
      * @brief Sets Node name
      * @param name Custom node name. This should be unique in the network
      */
    void setNodeName (const char* name) {
        memset (nodeName, 0, NODE_NAME_LENGTH);
        strncpy (nodeName, name, NODE_NAME_LENGTH);
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
      * @brief Gets counter for last received control message from node
      * @return Message counter
      */
    uint16_t getLastControlCounter () {
        return lastControlCounter;
    }

    /**
      * @brief Sets counter for last received message from node
      * @param counter Message counter
      */
    void setLastMessageCounter (uint16_t counter) {
        lastMessageCounter = counter;
    }

    /**
      * @brief Sets counter for last received control message from node
      * @param counter Message counter
      */
    void setLastControlCounter (uint16_t counter) {
        lastControlCounter = counter;
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
        if (initAsSleepy) {
            sleepyNode = sleepy;
        } else {
            sleepyNode = false;
        }
    }

	/**
	  * @brief Records if node started as a sleepy node or not. If it did not started so it will never accept sleep time changes
	  * @param sleepy `true` if node started as sleepy. `false` otherwise
	  */
    void setInitAsSleepy (bool sleepy) {
        initAsSleepy = sleepy;
    }

	/**
	  * @brief Gets initial sleepy mode
	  * @return `true` if node started as sleepy. `false` otherwise
	  */
    bool getInitAsSleepy () {
        return initAsSleepy;
    }

    /**
      * @brief Gets node working mode regarding battery saving strategy. If node is sleepy it will turn into deep sleep
      * after sending a message. In this case it will wait for a short while for a downlink message from gateway
      * @return `true` if node sleeps after sending a message and wait for downlink. `false` if downlink may happen in any moment
      */
    bool getSleepy () {
        return sleepyNode;
    }

    /**
      * @brief Adds a new message rate value for filter calculation
      * @param value Next value for calculation
      */
    void updatePacketsRate (float value);

    uint8_t queuedMessage[MAX_MESSAGE_LENGTH]; ///< @brief Message queued for sending to node in case of sleepy mode
    size_t qMessageLength;  ///< @brief Queued message length
    bool qMessagePending = false; ///< @brief `True` if message should be sent just after next data message

    uint32_t packetNumber = 0; ///< @brief Number of packets received from node to gateway
    uint32_t packetErrors = 0; ///< @brief Number of errored packets
    double per = 0;  ///< @brief Current packet error rate of a specific node
    double packetsHour = 0; ///< @brief Packet rate ffor a specific nope
    int64_t t1, t2, t3, t4;  ///< @brief Timestaps to calculate clock offset

protected:
//#define KEYLENGTH 32
    bool keyValid; ///< @brief Node shared key valid
    status_t status; ///< @brief Current node status. See `enum node_status`
    uint16_t lastMessageCounter; ///< @brief Last message counter state for specific Node
    uint16_t nodeId; ///< @brief Node identifier asigned by gateway
    timer_t keyValidFrom; ///< @brief Last time that Node and Gateway agreed a key
    bool sleepyNode = true; ///< @brief Node sleepy definition
    bool initAsSleepy; ///< @brief Stores initial sleepy node. If this is false, this node does not accept sleep time changes
    uint8_t mac[ENIGMAIOT_ADDR_LEN]; ///< @brief Node address
    uint8_t key[KEY_LENGTH]; ///< @brief Shared key
    timer_t lastMessageTime; ///< @brief Node state
    FilterClass* rateFilter; ///< @brief Filter for message rate smoothing
    char nodeName[NODE_NAME_LENGTH]; ///< @brief Node name. Use as a human friendly name to avoid use of numeric address*/

     /**
      * @brief Starts smoothing filter
      */
    void initRateFilter ();

    friend class NodeList;
};


class NodeList {
public:

    /**
      * @brief Node list constructor
      */
    NodeList ();

    /**
      * @brief Gets node that correspond with given nodeId
      * @param nodeId NodeId to search for
      * @return Node instance that has given nodeId. NULL if it was not found
      */
    Node *getNodeFromID (uint16_t nodeId);

    /**
      * @brief Gets node that correspond with given address
      * @param mac address to search for
      * @return Node instance that has given address. NULL if it was not found
      */
    Node *getNodeFromMAC (const uint8_t* mac);

     /**
      * @brief Gets node that correspond with given node name
      * @param name Node name to search for
      * @return Node instance that has given name. NULL if it was not found
      */
    Node* getNodeFromName (const char* name);
    
    /**
      * @brief Check Node name for duplicate
      * @param name Custom node name
      * @param address Address of node which is being tried to set name
      * @return Error code to show name correctness. 0 = OK, -1 = Name already used, -2 = Name is too long, -3 = Name is empty
      */
    int8_t checkNodeName (const char* name, const uint8_t* address);

    /**
      * @brief Searches for a free place for a new Node instance
      * @return Node instance to hold new instance
      */
    Node *findEmptyNode ();
    
     /**
      * @brief Gets the number of active nodes (registered or registering)
      * @return Number of active nodes
      */
    uint16_t countActiveNodes ();
    
    /**
      * @brief Frees up a node and marks it as available
      * @param nodeId NodeId to free up
      * @return 'True' if it was deleted. 'False' if nodeId was not found
      */
    bool unregisterNode (uint16_t nodeId);
    
    /**
      * @brief Frees up a node and marks it as available
      * @param mac Address to free up
      * @return 'True' if it was deleted. 'False' if address was not found
      */
    bool unregisterNode (const uint8_t* mac);
    
    /**
      * @brief Frees up a node using a pointer to it
      * @param node Pointer to node instance
      * @return 'True' if it was deleted. 'False' if it was already deleted
      */
    bool unregisterNode (Node *node);

    /**
      * @brief Gets next active node by nodeId
      * @param nodeId NodeId of the node to find
      * @return Pointer to node or NULL if it was not found
      */
    Node *getNextActiveNode (uint16_t nodeId);

    /**
      * @brief Gets next active node by instance where to get nodeId
      * @param node Node which have the nodeId to find
      * @return Pointer to node or NULL if it was not found
      */
    Node *getNextActiveNode (Node node);

     /**
      * @brief Finds a node that correspond with given address of creates a new one if it does not exist
      * @param mac address to search for
      * @return Node instance. NULL if it Node store is full
      */
    Node *getNewNode (const uint8_t* mac);

      /**
      * @brief Dumps node list data to a Stream object
      * @param port Stram port
      */
    void printToSerial (Stream *port);

protected:
    Node nodes[NUM_NODES]; ///< @brief Static Node array that holds maximum number of supported nodes 

};


#endif

