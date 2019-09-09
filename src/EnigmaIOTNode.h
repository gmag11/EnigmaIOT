/**
  * @file EnigmaIOTNode.h
  * @version 0.4.0
  * @date 10/09/2019
  * @author German Martin
  * @brief Library to build a node for EnigmaIoT system
  */

#ifndef _ENIGMAIOTNODE_h
#define _ENIGMAIOTNODE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "lib/EnigmaIoTconfig.h"
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "Comms_hal.h"
#include <CRC32.h>
#include "NodeList.h"
#include <cstddef>
#include <cstdint>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
//#include <ESP8266TrueRandom.h>

/**
  * @brief Message code definition
  */
enum nodeMessageType {
    SENSOR_DATA = 0x01, /**< Data message from sensor node */
    DOWNSTREAM_DATA = 0x02, /**< Data message from gateway. Downstream data for commands */
	CONTROL_DATA = 0x03, /**< Internal control message from node to gateway. Used for OTA, settings configuration, etc */
	DOWNSTREAM_CTRL_DATA = 0x04, /**< Internal control message from gateway to node. Used for OTA, settings configuration, etc */
    CLOCK_REQUEST = 0x05, /**< Clock request message from node */
    CLOCK_RESPONSE = 0x06, /**< Clock response message from gateway */
	CLIENT_HELLO = 0xFF, /**< ClientHello message from node */
    SERVER_HELLO = 0xFE, /**< ServerHello message from gateway */
    INVALIDATE_KEY = 0xFB /**< InvalidateKey message from gateway */
};

/**
  * @brief Key invalidation reason definition
  */
enum nodeInvalidateReason_t {
    UNKNOWN_ERROR = 0x00, /**< Unknown error. Not used by the moment */
    WRONG_CLIENT_HELLO = 0x01, /**< ClientHello message received was invalid */
    //WRONG_EXCHANGE_FINISHED = 0x02, /**< KeyExchangeFinished message received was invalid. Probably this means an error on shared key */
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
	uint8_t networkKey[KEY_LENGTH]; /**< Network key to protect key agreement */
	status_t nodeRegisterStatus = UNREGISTERED; /**< Node registration status */
	bool sleepy; /**< Sleepy node */
	uint32_t sleepTime = 0; /**< Time to sleep between sensor data delivery */
	bool nodeKeyValid = false; /**< true if key has been negotiated successfully */
};

typedef nodeMessageType nodeMessageType_t;

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
#include <functional>
typedef std::function<void (const uint8_t* mac, const uint8_t* buf, uint8_t len)> onNodeDataRx_t;
typedef std::function<void ()> onConnected_t;
typedef std::function<void ()> onDisconnected_t;
#else
typedef void (*onNodeDataRx_t)(const uint8_t*, const uint8_t*, uint8_t);
typedef void (*onConnected_t)();
typedef void (*onDisconnected_t)();
#endif

/**
  * @brief Main node class. Manages communication with gateway and allows sending and receiving user data
  *
  */
class EnigmaIOTNodeClass
{
protected:
    Node node; ///< @brief Node abstraction to store context
    bool flashBlue = false; ///< @brief If true Tx LED will be flashed
    int8_t led = -1; ///< @brief IO Pin that corresponds to Tx LED. Default value disables LED. It is initialized with `setLed` method
    unsigned int ledOnTime; ///< @brief Time that LED is On during flash. Initalized on `setLed`
    Comms_halClass *comm; ///< @brief Comms abstraction layer
    onNodeDataRx_t notifyData; ///< @brief Callback that will be called on every message reception
    onConnected_t notifyConnection; ///< @brief Callback that will be called anytime a new node is registered
    onDisconnected_t notifyDisconnection; ///< @brief Callback that will be called anytime a node is disconnected
    bool useCounter = true; ///< @brief `true` means that data message counter will be used to mark message order
    rtcmem_data_t rtcmem_data; ///< @brief Context data to be stored on persistent storage
    bool sleepRequested = false; ///< @brief `true` means that this node will sleep as soon a message is sent and downlink wait time has passed
    uint64_t sleepTime; ///< @brief Time in microseconds that this node will be slept between measurements
    uint8_t dataMessageSent[MAX_MESSAGE_LENGTH]; ///< @brief Buffer where sent message is stored in case of retransmission is needed
    uint8_t dataMessageSentLength = 0; ///< @brief Message length stored for use in case of message retransmission is needed
    nodeInvalidateReason_t invalidateReason = UNKNOWN_ERROR; ///< @brief Last key invalidation reason
	bool otaRunning = false; ///< @brief True if OTA update has started
	bool otaError = false; ///< @brief True if OTA update has failed. This normally produces a restart
	time_t lastOTAmsg; ///< @brief Time when last OTA update message has received. This is used to control timeout
	boolean indentifying = false; ///< @brief True if node has its led flashing to be identified
	time_t identifyStart; ///< @brief Time when identification started flashing. Used to control identification timeout

    /**
      * @brief Check that a given CRC matches to calulated value from a buffer
      * @param buf Pointer to the buffer that contains the stream to calculate CRC
      * @param count Buffer length in number of bytes 
      * @param crc Received CRC to check against calculation
      * @return Returns `true` if CRC check was successful. `false` otherwise  
      */
    bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc);
	
	/**
	  * @brief Starts node identification by flashing led
	  * @param period Flash led period in ms
	  */
	void startIdentifying (time_t period);

	/**
	  * @brief Stops node identification
	  */
	void stopIdentifying ();

	/**
   * @brief Loads configuration from RTC data. Uses a CRC to check data integrity
   * @return Returns `true` if data is valid. `false` otherwise
   */
	bool loadRTCData ();

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

	/**
	* @brief Starts configuration AP and web server and gets settings from it
    * @param data Pointer to configuration data to be stored on RTC memory to keep status
	*             along sleep cycles
	* @return Returns `true` if data was been correctly configured. `false` otherwise
	*/
	bool configWiFiManager (rtcmem_data_t* data);

	/**
	* @brief Sets connection as unregistered to force a resyncrhonisation after boot
	* @param reboot True if a reboot should be triggered after unregistration
	*/
	void restart (bool reboot = true);

    /**
      * @brief Build a **ClientHello** messange and send it to gateway
      * @return Returns `true` if ClientHello message was successfully sent. `false` otherwise
      */
    bool clientHello ();

    bool clockRequest ();

    bool processClockResponse (const uint8_t mac[6], const uint8_t* buf, size_t count);

    /**
      * @brief Gets a buffer containing a **ServerHello** message and process it. It uses that message to calculate a shared key using Diffie Hellman algorithm
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of ServerHello message
      * @return Returns `true` if message could be correcly processed
      */
    bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count);

    /**
      * @brief Gets a buffer containing an **InvalidateKey** message and process it. This trigger a new key agreement to start
      * @param mac Address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of InvalidateKey message
      * @return Returns the reason because key is not valid anymore. Check possible values in nodeInvalidateReason_t
      */
    nodeInvalidateReason_t processInvalidateKey (const uint8_t mac[6], const uint8_t* buf, size_t count);
    
    /**
      * @brief Builds, encrypts and sends a **Data** message.
      * @param data Buffer to store payload to be sent
      * @param len Length of payload data
	  * @param controlMessage Signals if this message is an EnigmaIoT control message that should not be passed to higher layers
      * @return Returns `true` if message could be correcly sent
      */
    bool dataMessage (const uint8_t *data, size_t len, bool controlMessage = false);

	/**
	  * @brief Processes a single OTA update command or data
	  * @param mac Gateway address
	  * @param data Buffer to store received message
	  * @param len Length of payload data
	  * @return Returns `true` if message could be correcly decoded and processed
	  */
	bool processOTACommand (const uint8_t* mac, const uint8_t* data, uint8_t len);

	/**
	  * @brief Processes a control command. Does not propagate to user code
	  * @param mac Gateway address
	  * @param data Buffer to store received message
	  * @param len Length of payload data
	  * @return Returns `true` if message could be correcly decoded and processed
	  */
	bool processControlCommand (const uint8_t mac[6], const uint8_t* data, size_t len);

    /**
      * @brief Processes downstream data from gateway
      * @param mac Gateway address
      * @param buf Buffer to store received payload
      * @param count Length of payload data
	  * @param control Idicates if downstream message is user or control data. If true it is a control message
      * @return Returns `true` if message could be correcly decoded
      */
    bool processDownstreamData (const uint8_t mac[6], const uint8_t* buf, size_t count, bool control = false);

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
	
	/**
	  * @brief Processes a request of sleep time configuration
	  * @param mac Gateway address
	  * @param buf Buffer to store received message
	  * @param len Length of payload data
	  * @return Returns `true` if message could be correcly decoded and processed
	  */
	bool processGetSleepTimeCommand (const uint8_t* mac, const uint8_t* buf, uint8_t len);
	
	/**
	  * @brief Processes a request to set new sleep time configuration
	  * @param mac Gateway address
	  * @param buf Buffer to store received message
	  * @param len Length of payload data
	  * @return Returns `true` if message could be correcly decoded and processed
	  */
	bool processSetSleepTimeCommand (const uint8_t* mac, const uint8_t* buf, uint8_t len);

	/**
	  * @brief Processes a request to start indicate to identify a node visually
	  * @param mac Gateway address
	  * @param buf Buffer to store received message
	  * @param len Length of payload data
	  * @return Returns `true` if message could be correcly decoded and processed
	  */
	bool processSetIdentifyCommand (const uint8_t* mac, const uint8_t* buf, uint8_t len);

	/**
	  * @brief Processes a request firmware version
	  * @param mac Gateway address
	  * @param buf Buffer to store received message
	  * @param len Length of payload data
	  * @return Returns `true` if message could be correcly decoded and processed
	  */
	bool processVersionCommand (const uint8_t* mac, const uint8_t* buf, uint8_t len);

	/**
	  * @brief Initiades data transmission distinguissing if it is payload or control data.
	  * @param data Buffer to store payload to be sent
	  * @param len Length of payload data
	  * @param controlMessage Signals if this message is an EnigmaIoT control message that should not be passed to higher layers
	  * @return Returns `true` if message could be correcly sent
	  */
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

	/**
	  * @brief Stops EnigmaIoT protocol
	  */
	void stop ();

	/**
	  * @brief Allows to configure a new sleep time period from user code
	  * @param sleepTime Time in seconds. Final period is not espected to be exact. Its value
	  *                  depends on communication process
	  */
	void setSleepTime (uint32_t sleepTime);

	/**
	  * @brief Returns sleep period in seconds
	  * @return Sleep period in seconds
	  */
	uint32_t getSleepTime () {
		if (!node.getSleepy()) {
			return 0;
		}
		else {
			return rtcmem_data.sleepTime;
		}
	}

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
      *   EnigmaIOTNode.onDataRx (processRxData);
      *   .....
      * }
      * 
      * void loop {
      *   .....
      * }
      * ```
      * @param handler Pointer to the function
      */
    void onDataRx (onNodeDataRx_t handler) {
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
      *   EnigmaIOTNode.onConnected (connectEventHandler);
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
      *   EnigmaIOTNode.onDisconnected (disconnectEventHandler);
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
      */
    void sleep ();

	/**
	  * @brief Gets current clock counter. millis() + offset
	  * @return Clock value in `time_t` format
	  */
	time_t clock ();

	/**
	  * @brief Checks if internal clock is synchronized to gateway
	  * @return True if clock is synchronized
	  */
	bool hasClockSync ();

	/**
	  * @brief Checks if node is registered
	  * @return True if node is registered
	  */
	bool isRegistered () {
		return node.isRegistered ();
	}

};

extern EnigmaIOTNodeClass EnigmaIOTNode;

#endif

