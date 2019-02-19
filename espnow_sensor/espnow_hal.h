// espnow_hal.h

#ifndef _ESPNOW_HAL_h
#define _ESPNOW_HAL_h

//#include <WifiEspNow.h>
#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "Comms_hal.h"

class Espnow_halClass : public Comms_halClass
{
 protected:
     uint8_t gateway[6];
     uint8_t channel;

     comms_hal_rcvd_data dataRcvd;
     comms_hal_sent_data sentResult;
     void initComms ();
     static void rx_cb (u8 *mac_addr, u8 *data, u8 len);
     static void tx_cb (u8 *mac_addr, u8 status);

 public:
	void begin(uint8_t* gateway, uint8_t channel);
    uint8_t send (u8 *da, u8 *data, int len);
    void onDataRcvd (comms_hal_rcvd_data dataRcvd);
    void onDataSent (comms_hal_sent_data dataRcvd);
    uint8_t getAddressLength () {
        return sizeof(gateway);
    }

};

extern Espnow_halClass Espnow_hal;

#endif

