/**
  * @file EnigmaIoTconfig.h
  * @version 0.2.0
  * @date 28/06/2019
  * @author German Martin
  * @brief Parameter configuration
  */

#ifndef _CONFIG_h
#define _CONFIG_h

// Global configuration
#define MAX_MESSAGE_LENGTH 250
static const char ENIGMAIOT_PROT_VERS[] = "0.2.0";
static const uint8_t DEFAULT_CHANNEL = 3;

// Gateway configuration
#define MAX_KEY_VALIDITY 86400000 ///< @brief 1 day
#define MAX_NODE_INACTIVITY 86400000U ///< @brief After this time (in ms) a node is marked as gone

// Sensor configuration
static const uint16_t RECONNECTION_PERIOD = 2500; ///< @brief Time to retry Gateway connection
static const uint16_t DOWNLINK_WAIT_TIME = 400;
static const uint32_t DEFAULT_SLEEP_TIME = 10; ///< @brief Default sleep time if it was not set
static const uint32_t OTA_TIMEOUT_TIME = 10000; ///< @brief Timeout between OTA messages. In milliseconds

//Crypro configuration
const uint8_t KEY_LENGTH = 32; ///< @brief Key length used by selected crypto algorythm. The only tested value is 32. Change it only if you know what you are doing
const uint8_t IV_LENGTH = 16; ///< @brief Initalization vector length used by selected crypto algorythm
#define BLOCK_CYPHER Speck
#define CYPHER_TYPE CFB<BLOCK_CYPHER>

//Debug
#define DEBUG_ESP_PORT Serial
#define DEBUG_LEVEL INFO // Possible values VERBOSE, DBG, INFO, WARN, ERROR, NONE

#endif