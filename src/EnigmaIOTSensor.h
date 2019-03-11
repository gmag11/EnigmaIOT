/**
  * @file EnigmaIOTSensor.h
  * @version 0.0.1
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

#include "lib/config.h"
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "comms_hal.h"
#include <CRC32.h>
#include "NodeList.h"
#include <cstddef>
#include <cstdint>

/**
  * @brief Message code definition
  */
enum sensorMessageType {
    SENSOR_DATA = 0x01, /**< Data message from sensor node */
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
    uint8_t nodeId; /**< Node identification */
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
    uint8_t gateway[6]; /**< Gateway MAC address to sent messages to */
    Node node; /**< Sensor node abstraction to store context */
    bool flashBlue = false; /**< If true Tx LED will be flashed */
    int8_t led = -1; /**< IO Pin that corresponds to Tx LED. Default value disables LED. It is initialized with `setLed` method */
    unsigned int ledOnTime; /**< Time that LED is On during flash. Initalized on `setLed` */
    uint8_t channel = 3; /**< Comms channel to transmit on */
    Comms_halClass *comm; /**< Comms abstraction layer */
    onSensorDataRx_t notifyData; /**< Callback that will be called on every message reception */
    onConnected_t notifyConnection; /**< Callback that will be called anytime a new node is registered */
    onDisconnected_t notifyDisconnection; /**< Callback that will be called anytime a node is disconnected */
    bool useCounter = true; /**< `true` means that data message counter will be used to mark message order */
    rtcmem_data_t rtcmem_data; /**< Context data to be stored on persistent storage */
    bool sleepRequested = false; /**< `true` means that this node will sleep as soon a message is sent and downlink wait time has passed */
    uint64_t sleepTime; /**< Time in microseconds that this node will be slept between measurements */
    uint8_t dataMessageSent[MAX_MESSAGE_LENGTH];
    uint8_t dataMessageSentLength = 0;
    sensorInvalidateReason_t invalidateReason = UNKNOWN_ERROR;

    /**
      * @brief Check that a given CRC matches to calulated value from a buffer
      * @param buf Pointer to the buffer that contains the stream to calculate CRC
      * @param count Buffer length in number of bytes 
      * @param crc Received CRC to check against calculation
      * @return Returns `true` if CRC check was successful. `false` otherwise  
      */
    bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc);

    /**
      * @brief Build a **ClientHello** messange and send it to gateway
      * @return Returns `true` if ClientHello message was successfully sent. `false` otherwise
      */
    bool clientHello ();

    /**
      * @brief Gets a buffer containing a **ServerHello** message and process it. It uses that message to calculate a shared key using Diffie Hellman algorithm
      * @param mac MAC address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of ServerHello message
      * @return Returns `true` if message syntax is correct and key was successfuly calculated `false` otherwise
      */
    bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count);

    /**
      * @brief Gets a buffer containing a **CipherFinished** message and process it. It checks that gateway key is correct by decrypting this message random payload
      * @param mac MAC address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of CipherFinished message
      * @return Returns `true` if message syntax is correct and key was successfuly calculated `false` otherwise
      */
    bool processCipherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count);
    
    /**
      * @brief Gets a buffer containing an **InvalidateKey** message and process it. This trigger a new key agreement to start
      * @param mac MAC address where this message was received from
      * @param buf Pointer to the buffer that contains the message
      * @param count Message length in number of bytes of InvalidateKey message
      * @return Returns the reason because key is not valid anymore. Check possible values in sensorInvalidateReason_t
      */
    sensorInvalidateReason_t processInvalidateKey (const uint8_t mac[6], const uint8_t* buf, size_t count);
    bool keyExchangeFinished ();
    bool dataMessage (const uint8_t *data, size_t len);
    void manageMessage (const uint8_t *mac, const uint8_t* buf, uint8_t count);
    void getStatus (u8 *mac_addr, u8 status);
    static void rx_cb (u8 *mac_addr, u8 *data, u8 len);
    static void tx_cb (u8 *mac_addr, u8 status);


public:
    void begin (Comms_halClass *comm, uint8_t *gateway, bool useCounter = true, bool sleepy = true);
    void handle ();
    void setLed (uint8_t led, time_t onTime = 100);
    bool sendData (const uint8_t *data, size_t len);
    void onDataRx (onSensorDataRx_t handler) {
        notifyData = handler;
    }
    void onConnected (onConnected_t handler) {
        notifyConnection = handler;
    }
    void onDisconnected (onDisconnected_t handler) {
        notifyDisconnection = handler;
    }
    void sleep (uint64_t time);
};

extern EnigmaIOTSensorClass EnigmaIOTSensor;

#endif

