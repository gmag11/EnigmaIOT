// Comms_hal.h

#ifndef _COMMS_HAL_h
#define _COMMS_HAL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

typedef void (*comms_hal_rcvd_data)(u8 *mac_addr, u8 *data, u8 len);
typedef void (*comms_hal_sent_data)(u8 *mac_addr, u8 status);

class Comms_halClass
{
 protected:
     comms_hal_rcvd_data dataRcvd = 0;
     comms_hal_sent_data sentResult = 0;
     virtual void initComms () = 0;


 public:
     virtual void begin (uint8_t* gateway, uint8_t channel) = 0;
     virtual uint8_t send (u8 *da, u8 *data, int len) = 0;
     virtual void onDataRcvd (comms_hal_rcvd_data dataRcvd) = 0;
     virtual void onDataSent (comms_hal_sent_data dataRcvd) = 0;
     virtual uint8_t getAddressLength () = 0;
};

#endif

