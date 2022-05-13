
#ifndef _ENIGMAIOTNODE_JSON_CONFIGURER_h
#define _ENIGMAIOTNODE_JSON_CONFIGURER_h

#include "rtc_data.h"

#ifndef JSON_CONFIGURER_FILE_NAME
#define JSON_CONFIGURER_FILE_NAME "/bootstrap.json"
#endif

/**
 * @brief read initial configuration from a json file
 */
class EnigmaIOTNodeJsonConfigurerClass {
   protected:
    //
   public:
    /**
     * @brief populate data from input json file
     * @param data pointer to TRC data structure
     */
    bool configure(rtcmem_data_t* data);
};

extern EnigmaIOTNodeJsonConfigurerClass EnigmaIOTNodeJsonConfigurer;

#endif  // _ENIGMAIOTNODE_JSON_CONFIGURER_h
