// SecureSensorGateway.h

#ifndef _SECURESENSORGATEWAY_h
#define _SECURESENSORGATEWAY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
//#include <WifiEspNow.h>
#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "comms_hal.h"
#include "NodeList.h"
#include <CRC32.h>
#include <cstddef>
#include <cstdint>

#define MAX_KEY_VALIDITY 60000
#define BLUE_LED 2
#define RED_LED 16


enum messageType_t {
    SENSOR_DATA = 0x01,
    CLIENT_HELLO = 0xFF,
    SERVER_HELLO = 0xFE,
    KEY_EXCHANGE_FINISHED = 0xFD,
    CYPHER_FINISHED = 0xFC,
    INVALIDATE_KEY = 0xFB
};

enum invalidateReason_t {
    WRONG_CLIENT_HELLO = 0x01,
    WRONG_EXCHANGE_FINISHED = 0x02,
    WRONG_DATA = 0x03,
    UNREGISTERED_NODE = 0x04,
    KEY_EXPIRED = 0x05
};


class SecureSensorGatewayClass
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
     int txLedOnTime;
     int rxLedOnTime;

     
     bool serverHello (const uint8_t *key, Node *node);
     bool checkCRC (const uint8_t *buf, size_t count, const uint32_t *crc);
     bool processClientHello (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);
     bool invalidateKey (Node *node, uint8_t reason);
     bool cipherFinished (Node *node);
     bool processKeyExchangeFinished (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);
     bool processDataMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node *node);
     void manageMessage (const uint8_t* mac, const uint8_t* buf, uint8_t count);
     static void rx_cb (u8 *mac_addr, u8 *data, u8 len);
     static void tx_cb (u8 *mac_addr, u8 status);
     void getStatus (u8 *mac_addr, u8 status);

 public:
     void begin (Comms_halClass *comm);
     void handle ();
     void setTxLed (uint8_t led, time_t onTime = 100);
     void setRxLed (uint8_t led, time_t onTime = 100);


};

extern SecureSensorGatewayClass SecureSensorGateway;

#endif

