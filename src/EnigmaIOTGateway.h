// EnigmaIOTGateway.h

#ifndef _ENIGMAIOTGATEWAY_h
#define _ENIGMAIOTGATEWAY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include "lib/config.h"
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "comms_hal.h"
#include "NodeList.h"
#include <CRC32.h>
#include <cstddef>
#include <cstdint>

enum gatewayMessageType_t {
    SENSOR_DATA = 0x01,
    DOWNSTREAM_DATA = 0x02,
    CLIENT_HELLO = 0xFF,
    SERVER_HELLO = 0xFE,
    KEY_EXCHANGE_FINISHED = 0xFD,
    CYPHER_FINISHED = 0xFC,
    INVALIDATE_KEY = 0xFB
};

enum gwInvalidateReason_t {
    UNKNOWN_ERROR = 0x00,
    WRONG_CLIENT_HELLO = 0x01,
    WRONG_EXCHANGE_FINISHED = 0x02,
    WRONG_DATA = 0x03,
    UNREGISTERED_NODE = 0x04,
    KEY_EXPIRED = 0x05
};

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
#include <functional>
typedef std::function<void (const uint8_t* mac, const uint8_t* buf, uint8_t len, uint16_t lostMessages)> onGwDataRx_t;
typedef std::function<void (uint8_t* mac)> onNewNode_t;
typedef std::function<void (uint8_t* mac, gwInvalidateReason_t reason)> onNodeDisconnected_t;
#else
typedef void (*onGwDataRx_t)(const uint8_t*, const uint8_t*, uint8_t, size_t);
typedef void (*onNewNode_t)(const uint8_t*);
typedef void (*onNodeDisconnected_t)(const uint8_t*, gwInvalidateReason_t);
#endif

class EnigmaIOTGatewayClass
{
 protected:
     uint8_t myPublicKey[KEY_LENGTH];
     bool flashTx = false;
     bool flashRx = false;
     node_t node;
     NodeList nodelist;
     Comms_halClass *comm;
     int8_t txled = -1;
     int8_t rxled = -1;
     unsigned long txLedOnTime;
     unsigned long rxLedOnTime;
     onGwDataRx_t notifyData;
     onNewNode_t notifyNewNode;
     onNodeDisconnected_t notifyNodeDisconnection;
     bool useCounter = true;
     
     bool serverHello (const uint8_t *key, Node *node);
     bool checkCRC (const uint8_t *buf, size_t count, const uint32_t *crc);
     bool processClientHello (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);
     bool invalidateKey (Node *node, gwInvalidateReason_t reason);
     bool cipherFinished (Node *node);
     bool processKeyExchangeFinished (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);
     bool processDataMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);
     bool downstreamDataMessage (Node *node, const uint8_t *data, size_t len);
     void manageMessage (const uint8_t* mac, const uint8_t* buf, uint8_t count);
     static void rx_cb (u8 *mac_addr, u8 *data, u8 len);
     static void tx_cb (u8 *mac_addr, u8 status);
     void getStatus (u8 *mac_addr, u8 status);

 public:
     void begin (Comms_halClass *comm, bool useDataCounter = true);
     void handle ();
     void setTxLed (uint8_t led, time_t onTime = 100);
     void setRxLed (uint8_t led, time_t onTime = 100);
     void onDataRx (onGwDataRx_t handler) {
         notifyData = handler;
     }

     /**
      * @brief Starts a downstream data message transmission
      * @param mac Node address
      * @param data Payload buffer
      * @param len Payload length
      */
     bool sendDownstream (uint8_t* mac, const uint8_t *data, size_t len);

     void onNewNode (onNewNode_t handler) {
         notifyNewNode = handler;
     }
     void onNodeDisconnected (onNodeDisconnected_t handler) {
         notifyNodeDisconnection = handler;
     }


};

extern EnigmaIOTGatewayClass EnigmaIOTGateway;

#endif

