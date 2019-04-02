/**
  * @file config.h
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief Parameter configuration
  */

#ifndef _CONFIG_h
#define _CONFIG_h

// Global configuration
#define MAX_MESSAGE_LENGTH 250

// Gateway configuration
#define MAX_KEY_VALIDITY 86400000

// Sensor configuration
const uint16_t RECONNECTION_PERIOD = 500;
const uint16_t DOWNLINK_WAIT_TIME = 500;



#endif