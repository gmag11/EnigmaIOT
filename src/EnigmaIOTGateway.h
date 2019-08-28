/**
  * @file EnigmaIOTGateway.h
  * @version 0.3.0
  * @date 28/08/2019
  * @author German Martin
  * @brief Library to build a gateway for EnigmaIoT system
  */

#ifndef _ENIGMAIOTGATEWAY_h
#define _ENIGMAIOTGATEWAY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include "lib/EnigmaIoTconfig.h"
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "Comms_hal.h"
#include "NodeList.h"
#include <CRC32.h>
#include <cstddef>
#include <cstdint>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

/**
  * @brief Message code definition
  */
enum gatewayMessageType_t {
    SENSOR_DATA = 0x01, /**< Data message from sensor node */
    DOWNSTREAM_DATA = 0x02, /**< Data message from gateway. Downstream data for user commands */
	CONTROL_DATA = 0x03, /**< Internal control message from sensor to gateway. Used for OTA, settings configuration, etc */
	DOWNSTREAM_CTRL_DATA = 0x04, /**< Internal control message from gateway to sensor. Used for OTA, settings configuration, etc */
    CLIENT_HELLO = 0xFF, /**< ClientHello message from sensor node */
    SERVER_HELLO = 0xFE, /**< ServerHello message from gateway */
    KEY_EXCHANGE_FINISHED = 0xFD, /**< KeyExchangeFinished message from sensor node */
    CYPHER_FINISHED = 0xFC, /**< CypherFinished message from gateway */
    INVALIDATE_KEY = 0xFB /**< InvalidateKey message from gateway */
};

/**
  * @brief Key invalidation reason definition
  */
enum gwInvalidateReason_t {
    UNKNOWN_ERROR = 0x00, /**< Unknown error. Not used by the moment */
    WRONG_CLIENT_HELLO = 0x01, /**< ClientHello message received was invalid */
    WRONG_EXCHANGE_FINISHED = 0x02, /**< KeyExchangeFinished message received was invalid. Probably this means an error on shared key */
    WRONG_DATA = 0x03, /**< Data message received could not be decrypted successfuly */
    UNREGISTERED_NODE = 0x04, /**< Data received from an unregistered node*/
    KEY_EXPIRED = 0x05 /**< Node key has reached maximum validity time */
};

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
#include <functional>
typedef std::function<void (const uint8_t* mac, const uint8_t* buf, uint8_t len, uint16_t lostMessages, bool control)> onGwDataRx_t;
typedef std::function<void (uint8_t* mac)> onNewNode_t;
typedef std::function<void (uint8_t* mac, gwInvalidateReason_t reason)> onNodeDisconnected_t;
#else
typedef void (*onGwDataRx_t)(const uint8_t* mac, const uint8_t* data, uint8_t len, uint16_t lostMessages, bool control);
typedef void (*onNewNode_t)(const uint8_t*);
typedef void (*onNodeDisconnected_t)(const uint8_t*, gwInvalidateReason_t);
#endif

typedef struct {
	uint8_t channel = DEFAULT_CHANNEL; /**< Channel used for communications*/
	uint8_t networkKey[KEY_LENGTH];   /**< Network key to protect key agreement*/
} gateway_config_t;

/**
  * @brief Main gateway class. Manages communication with nodes and sends data to upper layer
  *
  */
class EnigmaIOTGatewayClass
{
 protected:
     uint8_t myPublicKey[KEY_LENGTH]; ///< @brief Temporary public key store used during key agreement
     bool flashTx = false; ///< @brief `true` if Tx LED should flash
     bool flashRx = false; ///< @brief `true` if Rx LED should flash
     node_t node; ///< @brief temporary store to keep node data while processing a message
     NodeList nodelist; ///< @brief Node database that keeps status and shared keys
     Comms_halClass *comm; ///< @brief Instance of physical communication layer
     int8_t txled = -1; ///< @brief I/O pin to connect a led that flashes when gateway transmits data
     int8_t rxled = -1; ///< @brief I/O pin to connect a led that flashes when gateway receives data
     unsigned long txLedOnTime; ///< @brief Flash duration for Tx LED
     unsigned long rxLedOnTime; ///< @brief Flash duration for Rx LED
     onGwDataRx_t notifyData; ///< @brief Callback function that will be invoked when data is received fron a node
     onNewNode_t notifyNewNode; ///< @brief Callback function that will be invoked when a new node is connected
     onNodeDisconnected_t notifyNodeDisconnection; ///< @brief Callback function that will be invoked when a node gets disconnected
     bool useCounter = true; ///< @brief `true` if counter is used to check data messages order
	 gateway_config_t gwConfig; ///< @brief Gateway specific configuration to be stored on flash memory
     
     /**
      * @brief Build a **ServerHello** messange and send it to node
      * @param key Node public key to be used on Diffie Hellman algorithm
      * @param node Entry in node list database where node will be registered
      * @return Returns `true` if ServerHello message was successfully sent. `false` otherwise
      */
     bool serverHello (const uint8_t *key, Node *node);

     /**
      * @brief Check that a given CRC matches to calulated value from a buffer
      * @param buf Pointer to the buffer that contains the stream to calculate CRC
      * @param count Buffer length in number of bytes
      * @param crc Received CRC to check against calculation
      * @return Returns `true` if CRC check was successful. `false` otherwise
      */
     bool checkCRC (const uint8_t *buf, size_t count, const uint32_t *crc);

     /**
      * @brief Gets a buffer containing a **ClientHello** message and process it. This carries node public key to be used on Diffie Hellman algorithm
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of ClientHello message
      * @param node Node entry that Client Hello message comes from
      * @return Returns `true` if message could be correcly processed
      */
     bool processClientHello (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);

     /**
      * @brief Creates an **InvalidateKey** message and sned it. This trigger a new key agreement to start on related node
      * @param node Node to send Invalidate Key message to
      * @param reason Reason that produced key invalidation in gwInvalidateReason_t format
      * @return Returns `true` if message could be correcly sent
      */
     bool invalidateKey (Node *node, gwInvalidateReason_t reason);

     /**
      * @brief Build and send a **cipherFinished** message.
      *
      * It includes some random data that is encrypted with a CRC to check integrity. This message is used to let node if gateway
      * generated the correct shared key.
      *
      * @param node Node which key agreement is being made with
      * @return Returns `true` if message could be correcly sent
      */
     bool cipherFinished (Node *node);

     /**
      * @brief Gets a buffer containing a **KeyExchangeFinished** message and process it. It checks that node key is correct by
      * decrypting this message random payload
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of CipherFinished message
      * @param node Node entry on database
      * @return Returns `true` if message syntax is correct and key was successfuly calculated `false` otherwise
      */
     bool processKeyExchangeFinished (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);

     /**
      * @brief Processes data message from node
      * @param mac Node address
      * @param buf Buffer that stores received message
      * @param count Length of received data
      * @param node Node where data message comes from
      * @return Returns `true` if message could be correcly decoded
      */
     bool processDataMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);

	 ///**
	 // * @brief Processes OTA update message
	 // * @param msg Payload, including node address
	 // * @param msgLen Length of received data
	 // * @param output 
	 // * @return Returns `true` if message could be correcly decoded and processed
	 // */
	 //bool processOTAMessage (uint8_t* msg, size_t msgLen, uint8_t* output);

     /**
      * @brief Builds, encrypts and sends a **DownstreamData** message.
      * @param node Node that downstream data message is going to
      * @param data Buffer to store payload to be sent
      * @param len Length of payload data
	  * @param controlData Content data type if control data
      * @return Returns `true` if message could be correcly sent or scheduled
      */
	 bool downstreamDataMessage (Node* node, const uint8_t* data, size_t len, control_message_type_t controlData = USERDATA);

	 /**
	 * @brief Processes control message from node
	 * @param mac Node address
	 * @param buf Buffer that stores received message
	 * @param count Length of received data
	 * @param node Node where data message comes from
	 * @return Returns `true` if message could be correcly decoded
	 */
	 bool processControlMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node);

     /**
      * @brief Process every received message.
      *
      * It starts clasiffying message usint the first byte. After that it passes it to the corresponding method for decoding
      * @param mac Address of message sender
      * @param buf Buffer that stores message bytes
      * @param count Length of message in number of bytes
      */
     void manageMessage (const uint8_t* mac, const uint8_t* buf, uint8_t count);

     /**
      * @brief Function that will be called anytime this gateway receives a message
      * @param mac_addr Address of message sender
      * @param data Buffer that stores message bytes
      * @param len Length of message in number of bytes
      */
     static void rx_cb (u8 *mac_addr, u8 *data, u8 len);

     /**
      * @brief Function that will be called anytime this gateway sends a message
      * to indicate status result of sending process
      * @param mac_addr Address of message destination
      * @param status Result of sending process
      */
     static void tx_cb (u8 *mac_addr, u8 status);

     /**
      * @brief Functrion to debug send status.
      * @param mac_addr Address of message sender
      * @param status Result status code
      */
     void getStatus (u8 *mac_addr, u8 status);

	 /**
	 * @brief Starts configuration AP and web server and gets settings from it
	 * @return Returns `true` if data was been correctly configured. `false` otherwise
	 */
	 bool configWiFiManager ();

	 /**
	 * @brief Loads configuration from flash memory
	 * @return Returns `true` if data was read successfuly. `false` otherwise
	 */
	 bool loadFlashData ();

	 /**
	 * @brief Saves configuration to flash memory
	 * @return Returns `true` if data could be written successfuly. `false` otherwise
	 */
	 bool saveFlashData ();

 public:
     /**
      * @brief Initalizes communication basic data and starts accepting node registration
      * @param comm Physical layer to be used on this network
      * @param networkKey Network key to protect shared key agreement
      * @param useDataCounter Indicates if a counter is going to be added to every message data to check message sequence. `true` by default
      */
     void begin (Comms_halClass *comm, uint8_t *networkKey = NULL, bool useDataCounter = true);

     /**
      * @brief This method should be called periodically for instance inside `loop()` function.
      * It is used for internal gateway maintenance tasks
      */
     void handle ();

     /**
      * @brief Sets a LED to be flashed every time a message is transmitted
      * @param led LED I/O pin
      * @param onTime Flash duration. 100ms by default.
      */
     void setTxLed (uint8_t led, time_t onTime = 100);

     /**
      * @brief Sets a LED to be flashed every time a message is received
      * @param led LED I/O pin
      * @param onTime Flash duration. 100ms by default.
      */
     void setRxLed (uint8_t led, time_t onTime = 100);

     /**
      * @brief Defines a function callback that will be called on every downlink data message that is received from a node
      *
      * Use example:
      * ``` C++
      * // First define the callback function
      * void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages) {
      *   // Do whatever you need with received data
      * }
      *
      * void setup () {
      *   .....
      *   // Now register function as data message handler
      *   EnigmaIOTSensor.onDataRx (processRxData);
      *   .....
      * }
      *
      * void loop {
      *   .....
      *   EnigmaIOTSensor.handle();
      *   .....
      * }
      * ```
      * @param handler Pointer to the function
      */
     void onDataRx (onGwDataRx_t handler) {
         notifyData = handler;
     }

     /**
      * @brief Gets packet error rate of node that has a specific address
      * @param address Node address
      * @return Packet error rate
      */
     double getPER (uint8_t *address);

     /**
      * @brief Gets total packets sent by node that has a specific address
      * @param address Node address
      * @return Node total sent packets
      */
     uint32_t getTotalPackets (uint8_t *address);

     /**
      * @brief Gets number of errored packets of node that has a specific address
      * @param address Node address
      * @return Node errored packets
      */
     uint32_t getErrorPackets (uint8_t *address);

     /**
      * @brief Gets packet rate sent by node that has a specific address, in packets per hour
      * @param address Node address
      * @return Node packet rate
      */
     double getPacketsHour (uint8_t *address);

     /**
      * @brief Starts a downstream data message transmission
      * @param mac Node address
      * @param data Payload buffer
      * @param len Payload length
      */
     bool sendDownstream (uint8_t* mac, const uint8_t *data, size_t len, control_message_type_t controlData = USERDATA);

     /**
      * @brief Defines a function callback that will be called every time a node gets connected or reconnected
      *
      * Use example:
      * ``` C++
      * // First define the callback function
      * void newNodeConnected (uint8_t* mac) {
      *   // Do whatever you need new node address
      * }
      *
      * void setup () {
      *   .....
      *   // Now register function as new node condition handler
      *   EnigmaIOTGateway.onNewNode (newNodeConnected);
      *   .....
      * }
      *
      * void loop {
      *   .....
      *   EnigmaIOTSensor.handle();
      *   .....
      * }
      * ```
      * @param handler Pointer to the function
      */
     void onNewNode (onNewNode_t handler) {
         notifyNewNode = handler;
     }

     /**
      * @brief Defines a function callback that will be called every time a node is disconnected
      *
      * Use example:
      * ``` C++
      * // First define the callback function
      * void nodeDisconnected (uint8_t* mac, gwInvalidateReason_t reason) {
      *   // Do whatever you need node address and disconnection reason
      * }
      *
      * void setup () {
      *   .....
      *   // Now register function as new node condition handler
      *   EnigmaIOTGateway.onNodeDisconnected (nodeDisconnected);
      *   .....
      * }
      *
      * void loop {
      *   .....
      *   EnigmaIOTSensor.handle();
      *   .....
      * }
      * ```
      * @param handler Pointer to the function
      */
     void onNodeDisconnected (onNodeDisconnected_t handler) {
         notifyNodeDisconnection = handler;
     }


};

extern EnigmaIOTGatewayClass EnigmaIOTGateway;

#endif

