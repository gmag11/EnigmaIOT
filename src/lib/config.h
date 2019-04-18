/**
  * @file config.h
  * @version 0.1.0
  * @date 09/03/2019
  * @author German Martin
  * @brief Parameter configuration
  */

#ifndef _CONFIG_h
#define _CONFIG_h

// Global configuration
#define MAX_MESSAGE_LENGTH 250

// Gateway configuration
#define MAX_KEY_VALIDITY 86400000 ///< @brief 1 day

// Sensor configuration
static const uint16_t RECONNECTION_PERIOD = 2500;
static const uint16_t DOWNLINK_WAIT_TIME = 500;

static const uint8_t NETWORK_KEY[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


#endif