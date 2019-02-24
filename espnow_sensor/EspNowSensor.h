// EspNowSensor.h

#ifndef _ESPNOWSENSOR_h
#define _ESPNOWSENSOR_h

//#include <WifiEspNow.h>
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "lib/cryptModule.h"
#include "lib/helperFunctions.h"
#include "comms_hal.h"
#include <CRC32.h>
#include "Node.h"
#include <cstddef>
#include <cstdint>

enum messageType {
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

typedef messageType messageType_t;

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
#include <functional>
typedef std::function<void (const uint8_t*, const uint8_t*, uint8_t)> onDataRx_t;
#else
typedef void (*onDataRx_t)(const uint8_t*, const uint8_t*, uint8_t);
#endif

class EspNowSensorClass
{
protected:
    uint8_t gateway[6] = { 0x5E, 0xCF, 0x7F, 0x80, 0x34, 0x75 };
    Node node;
    bool flashBlue = false;
    int flashLed = 2;
    int8_t led = -1;
    int ledOnTime;
    uint8_t channel = 3;
    Comms_halClass *comm;
    onDataRx_t notifyData;
    bool useCounter = false;

    bool checkCRC (const uint8_t *buf, size_t count, uint32_t *crc);
    bool clientHello (/*const uint8_t *key*/);
    bool processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count);
    bool processCipherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count);
    bool processInvalidateKey (const uint8_t mac[6], const uint8_t* buf, size_t count);
    bool keyExchangeFinished ();
    bool dataMessage (const uint8_t *data, size_t len);
    void manageMessage (const uint8_t *mac, const uint8_t* buf, uint8_t count);
    void getStatus (u8 *mac_addr, u8 status);
    static void rx_cb (u8 *mac_addr, u8 *data, u8 len);
    static void tx_cb (u8 *mac_addr, u8 status);


public:
    void begin (Comms_halClass *comm, bool useCounter = false);
    void handle ();
    void setLed (uint8_t led, time_t onTime = 100);
    bool sendData (const uint8_t *data, size_t len);
    void onDataRx (onDataRx_t handler) {
        notifyData = handler;
    }
};

extern EspNowSensorClass EspNowSensor;

#endif

