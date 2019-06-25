/**
  * @file EnigmaIOTSensor.h
  * @version 0.1.0
  * @date 09/03/2019
  * @author German Martin
  * @brief Library to build a node for EnigmaIoT system
  */

#ifndef _ENIGMAIOTSENSOR_h
#define _ENIGMAIOTSENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "lib/EnigmaIoTconfig.h"
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "comms_hal.h"
#include <CRC32.h>
#include "NodeList.h"
#include <cstddef>
#include <cstdint>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

/**
  * @brief Message code definition
  */
enum sensorMessageType {
    SENSOR_DATA = 0x01, /**< Data message from sensor node */
    DOWNSTREAM_DATA = 0x02, /**< Data message from gateway. Downstream data for commands */
	CONTROL_DATA = 0x03, /**< Internal ontrol message like OTA, settings configuration, etc */
	CLIENT_HELLO = 0xFF, /**< ClientHello message from sensor node */
    SERVER_HELLO = 0xFE, /**< ServerHello message from gateway */
    KEY_EXCHANGE_FINISHED = 0xFD, /**< KeyExchangeFinished message from sensor node */
    CYPHER_FINISHED = 0xFC, /**< CypherFinished message from gateway */
    INVALIDATE_KEY = 0xFB /**< InvalidateKey message from gateway */
};

/**
  * @brief Key invalidation reason definition
  */
enum sensorInvalidateReason_t {
    UNKNOWN_ERROR = 0x00, /**< Unknown error. Not used by the moment */
    WRONG_CLIENT_HELLO = 0x01, /**< ClientHello message received was invalid */
    WRONG_EXCHANGE_FINISHED = 0x02, /**< KeyExchangeFinished message received was invalid. Probably this means an error on shared key */
    WRONG_DATA = 0x03, /**< Data message received could not be decrypted successfuly */
    UNREGISTERED_NODE = 0x04, /**< Data received from an unregistered node*/
    KEY_EXPIRED = 0x05 /**< Node key has reached maximum validity time */
};

/**
  * @brief Context data to be stored con persistent storage to be used after wake from sleep mode
  */
struct rtcmem_data_t {
    uint32_t crc32; /**< CRC to check RTC data integrity */
    uint8_t nodeKey[KEY_LENGTH]; /**< Node shared key */
    uint16_t lastMessageCounter; /**< Node last message counter */
	uint16_t nodeId; /**< Node identification */
	uint8_t channel = DEFAULT_CHANNEL; /**< WiFi channel used on ESP-NOW communication */
	uint8_t gateway[6]; /**< Gateway address */
	uint8_t networkKey[KEY_LENGTH]; /**< Network key to encrypt dynamic key nogotiation */
	status_t nodeRegisterStatus = UNREGISTERED; /**< Node registration status */
	bool sleepy; /**< Sleepy node */
	bool nodeKeyValid = false; /**< true if key has been negotiated successfully */
};

typedef sensorMessageType sensorMessageType_t;

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
#include <functional>
typedef std::function<void (const uint8_t* mac, const uint8_t* buf, uint8_t len)> onSensorDataRx_t;
typedef std::function<void ()> onConnected_t;
typedef std::function<void ()> onDisconnected_t;
#else
typedef void (*onSensorDataRx_t)(const uint8_t*, const uint8_t*, uint8_t);
typedef void (*onConnected_t)();
typedef void (*onDisconnected_t)();
#endif

/**
  * @brief Main sensor node class. Manages communication with gateway and allows sending and receiving user data
  *
  */
class EnigmaIOTSensorClass
{
protected:
    //uint8_t gateway[6]; ///< @brief Gateway address to sent messages to
    Node node; ///< @brief Sensor node abstraction to store context
    bool flashBlue = false; ///< @brief If true Tx LED will be flashed
    int8_t led = -1; ///< @brief IO Pin that corresponds to Tx LED. Default value disables LED. It is initialized with `setLed` method
    unsigned int ledOnTime; ///< @brief Time that LED is On during flash. Initalized on `setLed`
    //uint8_t channel = 3; ///< @brief Comms channel to transmit on
    Comms_halClass *comm; ///< @brief Comms abstraction layer
    onSensorDataRx_t notifyData; ///< @brief Callback that will be called on every message reception
    onConnected_t notifyConnection; ///< @brief Callback that will be called anytime a new node is registered
    onDisconnected_t notifyDisconnection; ///< @brief Callback that will be called anytime a node is disconnected
    bool useCounter = true; ///< @brief `true` means that data message counter will be used to mark message order
    rtcmem_data_t rtcmem_data; ///< @brief Context data to be stored on persistent storage
    bool sleepRequested = false; ///< @brief `true` means that this node will sleep as soon a message is sent and downlink wait time has passed
    uint64_t sleepTime; ///< @brief Time in microseconds that this node will be slept between measurements
    uint8_t dataMessageSent[MAX_MESSAGE_LENGTH]; ///< @brief Buffer where sent message is stored in case of retransmission is needed
    uint8_t dataMessageSentLength = 0; ///< @brief Message length stored for use in case of message retransmission is needed
    sensorInvalidateReason_t invalidateReason = UNKNOWN_ERROR; ///< @brief Last key invalidation reason
    //uint8_t networkKey[KEY_LENGTH];   ///< @brief Network key to protect key agreement

    /**
      * @brief Check that a given CRC matches to calulated value from a buffer
      * @param buf Pointer to the buffer that contains the stream to calculate CRC
      * @param count Buffer length in number of bytes 
      * @param crc Received CRC to check against calculation
      * @return Returns `true` if CRC check was successful. `false` otherwise  
      */
    bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc);
	
	bool loadRTCData ();

	bool loadFlashData ();

	bool saveFlashData ();

	bool configWiFiManager (rtcmem_data_t* data);

    /**
      * @brief Build a **ClientHello** messange and send it to gateway
      * @return Returns `true` if ClientHello message was successfully sent. `false` otherwise
      */
    bool clientHello ();

    /**
      * @brief Gets a buffer containing a **ServerHello** message and process it. It uses that message to calculate a shared key using Diffie Hellman algorithm
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of ServerHello message
      * @return Returns `true` if message could be correcly processed
      */
    bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count);

    /**
      * @brief Gets a buffer containing a **CipherFinished** message and process it. It checks that gateway key is correct by decrypting this message random payload
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of CipherFinished message
      * @return Returns `true` if message syntax is correct and key was successfuly calculated `false` otherwise
      */
    bool processCipherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count);
    
    /**
      * @brief Gets a buffer containing an **InvalidateKey** message and process it. This trigger a new key agreement to start
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of InvalidateKey message
      * @return Returns the reason because key is not valid anymore. Check possible values in sensorInvalidateReason_t
      */
    sensorInvalidateReason_t processInvalidateKey (const uint8_t mac[6], const uint8_t* buf, size_t count);
    
    /**
      * @brief Build and send a **keyExchangeFinished** message. 
      * 
      * It includes some random data that is encrypted with a CRC to check integrity. This message is used to let Gateway if this node
      * generated the correct shared key.
      * @return Returns `true` if message could be correcly sent
      */
    bool keyExchangeFinished ();

    /**
      * @brief Builds, encrypts and sends a **Data** message.
      * @param data Buffer to store payload to be sent
      * @param len Length of payload data
      * @return Returns `true` if message could be correcly sent
      */
    bool dataMessage (const uint8_t *data, size_t len, bool controlMessage = false);

    /**
      * @brief Processes downstream data from gateway
      * @param mac Gateway address
      * @param buf Buffer to store received payload
      * @param count Length of payload data
      * @return Returns `true` if message could be correcly decoded
      */
    bool processDownstreamData (const uint8_t mac[6], const uint8_t* buf, size_t count);

    /**
      * @brief Process every received message.
      *
      * It starts clasiffying message usint the first byte. After that it passes it to the corresponding method for decoding 
      * @param mac Address of message sender
      * @param buf Buffer that stores message bytes
      * @param count Length of message in number of bytes
      */
    void manageMessage (const uint8_t *mac, const uint8_t* buf, uint8_t count);

    /**
      * @brief Functrion to debug send status.
      * @param mac_addr Address of message sender
      * @param status Result status code
      */
    void getStatus (uint8_t *mac_addr, uint8_t status);

    /**
      * @brief Function that will be called anytime this node receives a message
      * @param mac_addr Address of message sender
      * @param data Buffer that stores message bytes
      * @param len Length of message in number of bytes
      */
    static void rx_cb (uint8_t *mac_addr, uint8_t *data, uint8_t len);

    /**
      * @brief Function that will be called anytime this node sends a message
      * to indicate status result of sending process
      * @param mac_addr Address of message destination
      * @param status Result of sending process
      */
    static void tx_cb (uint8_t *mac_addr, uint8_t status);

	bool processVersionCommand (const uint8_t* mac, const uint8_t* buf, uint8_t len);

	/**
	  * @brief Processes internal sensor commands like
	  * version information, OTA, settings tuning, etc
	  * @param mac_addr Address of message sender
	  * @param buf Message payload
	  * @param len Payload length
	  */
	bool checkControlCommand (const uint8_t* mac, const uint8_t* buf, uint8_t len);

	bool sendData (const uint8_t* data, size_t len, bool controlMessage);

public:
    /**
      * @brief Initalizes communication basic data and starts node registration
      * @param comm Physical layer to be used on this node network
      * @param gateway Gateway address
      * @param networkKey Network key to protect shared key agreement
      * @param useCounter Indicates if a counter has to be added to every message data to check message sequence. `true` by default
      * @param sleepy Indicates if this node changes to low energy mode (sleep mode) after sending a data message. `true` by default
      *
      * This condition is signalled to Gateway during registration so that downlink messages are managed diferently.
      * On non sleepy nodes a downlink data message can be sent on any moment as node will be always awake. But for nodes that sleep,
      * normally those that are powered with batteries, downlink message will be queued on gateway and sent just after an uplink data
      * message from node has been sent
      */
    void begin (Comms_halClass *comm, uint8_t *gateway = NULL, uint8_t *networkKey = NULL, bool useCounter = true, bool sleepy = true);

	void stop ();

    /**
      * @brief This method should be called periodically for instance inside `loop()` function.
      * It is used for internal node maintenance tasks
      */
    void handle ();

    /**
      * @brief Sets a LED to be flashed every time a message is transmitted
      * @param led LED I/O pin
      * @param onTime Flash duration. 100ms by default.
      */
    void setLed (uint8_t led, time_t onTime = 100);

    /**
      * @brief Starts a data message transmission
      * @param data Payload buffer
      * @param len Payload length
      */
	bool sendData (const uint8_t* data, size_t len) {
		return sendData (data, len, false);
	}

    /**
      * @brief Defines a function callback that will be called on every downlink data message that is received from gateway
      *
      * Use example:
      * ``` C++
      * // First define the callback function
      * void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length) {
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
      * }
      * ```
      * @param handler Pointer to the function
      */
    void onDataRx (onSensorDataRx_t handler) {
        notifyData = handler;
    }

    /**
      * @brief Defines a function callback that will be called everytime node is registered on gateway.
      *
      * Use example:
      * ``` C++
      * // First define the callback function
      * void connectEventHandler () {
      *   // Do whatever you need to process disconnecion
      * }
      *
      * void setup () {
      *   .....
      *   // Now register function as data message handler
      *   EnigmaIOTSensor.onConnected (connectEventHandler);
      *   .....
      * }
      *
      * void loop {
      *   .....
      * }
      * ```
      * @param handler Pointer to the function
      */
    void onConnected (onConnected_t handler) {
        notifyConnection = handler;
    }

    /**
      * @brief Defines a function callback that will be called everytime node is disconnected from gateway.
      *
      * Deregistration is always started by gateway due syntax or encryption error or in case of key validity is over.
      *
      * Use example:
      * ``` C++
      * // First define the callback function
      * void disconnectEventHandler () {
      *   // Do whatever you need to process disconnecion
      * }
      *
      * void setup () {
      *   .....
      *   // Now register function as data message handler
      *   EnigmaIOTSensor.onDisconnected (disconnectEventHandler);
      *   .....
      * }
      *
      * void loop {
      *   .....
      * }
      * ```
      * @param handler Pointer to the function
      */
    void onDisconnected (onDisconnected_t handler) {
        notifyDisconnection = handler;
    }

    /**
      * @brief Requests transition to sleep mode (low energy state)
      *
      * Sleep can be requested in any moment and will be triggered inmediatelly except if node is doing registration or is waiting for downlink
      * @param time Sleep mode state duration
      */
    void sleep (uint64_t time);
};

extern EnigmaIOTSensorClass EnigmaIOTSensor;

#endif

