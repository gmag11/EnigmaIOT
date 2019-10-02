/**
  * @file EnigmaIoTconfig.h
  * @version 0.4.0
  * @date 10/09/2019
  * @author German Martin
  * @brief Parameter configuration
  */

#ifndef _CONFIG_h
#define _CONFIG_h

// Global configuration. Physical layer settings
static const uint8_t MAX_MESSAGE_LENGTH = 250; ///< @brief Maximum payload size on ESP-NOW
static const char ENIGMAIOT_PROT_VERS[] = "0.4.0"; ///< @brief EnitmaIoT Version
static const uint8_t DEFAULT_CHANNEL = 3; ///< @brief WiFi channel to be used on ESP-NOW
static const uint32_t FLASH_LED_TIME = 50; ///< @brief Time that led keeps on during flash in ms
static const uint8_t NETWORK_NAME_LENGTH = 21; ///< @brief Maximum number of characters of network name

// Gateway configuration
static const unsigned int MAX_KEY_VALIDITY = 86400000U; ///< @brief After this time (in ms) a nude is unregistered.
static const unsigned int MAX_NODE_INACTIVITY = 86400000U; ///< @brief After this time (in ms) a node is marked as gone

// Node configuration
static const int16_t RECONNECTION_PERIOD = 1500; ///< @brief Time to retry Gateway connection
static const uint16_t DOWNLINK_WAIT_TIME = 400; ///< @brief Time to wait for downlink message before sleep
static const uint32_t DEFAULT_SLEEP_TIME = 10; ///< @brief Default sleep time if it was not set
static const uint32_t OTA_TIMEOUT_TIME = 10000; ///< @brief Timeout between OTA messages. In milliseconds
static const time_t IDENTIFY_TIMEOUT = 10000; ///< @brief How long LED will be flashing during identification
static const uint32_t TIME_SYNC_PERIOD = 300000; ///< @brief Period of clock synchronization request
static const unsigned int QUICK_SYNC_TIME = 5000; ///< @brief Period of clock synchronization request in case of resync is needed 
static const int MIN_SYNC_ACCURACY = 5; ///< @brief If calculated offset absolute value is higher than this value resync is done more often
static const int MAX_DATA_PAYLOAD_SIZE = 215; ///< @brief Maximun payload size for data packets
static const uint32_t PRE_REG_DELAY = 5000; ///< @brief Time to wait before registration so that other nodes have time to communicate. Real delay is a random lower than this value.
static const uint32_t POST_REG_DELAY = 1500; ///< @brief Time to waif before sending data after registration so that other nodes have time to finish their registration. Real delay is a random lower than this value.
static const uint8_t COMM_ERRORS_BEFORE_SCAN = 2; ///< @brief Node will search for a gateway if this number of communication errors have happened.

//Crypto configuration
const uint8_t KEY_LENGTH = 32; ///< @brief Key length used by selected crypto algorythm. The only tested value is 32. Change it only if you know what you are doing
const uint8_t IV_LENGTH = 12; ///< @brief Initalization vector length used by selected crypto algorythm
const uint8_t TAG_LENGTH = 16; ///< @brief Authentication tag length. For Poly1305 it is always 16
const uint8_t AAD_LENGTH = 8; ///< @brief Number of bits from last part of key that will be used for additional authenticated data
#define CYPHER_TYPE ChaChaPoly

//Debug
#define DEBUG_ESP_PORT Serial ///< @brief Stream to output debug info. It will normally be `Serial`
#define DEBUG_LEVEL WARN ///< @brief Possible values VERBOSE, DBG, INFO, WARN, ERROR, NONE

#endif