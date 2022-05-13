
#ifndef _ENIGMAIOTNODE_SERIAL_CONFIGURER_h
#define _ENIGMAIOTNODE_SERIAL_CONFIGURER_h

#include "rtc_data.h"

/**
 * @brief read initial configuration from a Serial port
 */
class EnigmaIOTNodeSerialConfigurerClass {
   protected:
    //
   public:
    /**
     * @brief populate data from Serial port
     * @param data pointer to TRC data structure
     */
    bool configure(rtcmem_data_t* data);
};

extern EnigmaIOTNodeSerialConfigurerClass EnigmaIOTNodeSerialConfigurer;

#endif  // _ENIGMAIOTNODE_SERIAL_CONFIGURER_h
