/**
  * @file EnigmaIoTconfig.h
  * @version 0.9.7
  * @date 04/02/2021
  * @author German Martin
  * @brief Parameter configuration
  */

#ifndef _CONFIG_h
#define _CONFIG_h

#include "Arduino.h"
#include "EnigmaIoTconfigAdvanced.h"

// Global configuration. Physical layer settings
static const uint8_t ENIGMAIOT_PROT_VERS[3] = { 0,9,8 }; ///< @brief EnitmaIoT Version
static const uint8_t DEFAULT_CHANNEL = 3; ///< @brief WiFi channel to be used on ESP-NOW
static const uint32_t FLASH_LED_TIME = 30; ///< @brief Time that led keeps on during flash in ms
static const int RESET_PIN_DURATION = 5000; ///< @brief Number of milliseconds that reset pin has to be grounded to produce a configuration reset
#define TZINFO "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" ///< @brief Time zone
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"

// Gateway configuration
static const unsigned int MAX_KEY_VALIDITY = 172800000U; ///< @brief After this time (in ms) a node is unregistered. Setting this to 0 means imfinite
static const unsigned int MAX_NODE_INACTIVITY = 86400000U; ///< @brief After this time (in ms) a node is marked as gone. Setting this to 0 means imfinite
static const size_t MAX_MQTT_QUEUE_SIZE = 3; ///< @brief Maximum number of MQTT messages to be sent
#define ENABLE_STATUS_MESSAGES 1 ///< @brief Enable sending status message after every data message
static const int RATE_AVE_ORDER = 5; ///< @brief Message rate filter order
static const int MAX_INPUT_QUEUE_SIZE = 3; ///< @brief Input queue size for EnigmaIOT messages. Acts as a buffer to be able to handle messages during high load
#ifndef NUM_NODES
static const int NUM_NODES = 35; ///< @brief Maximum number of nodes that this gateway can handle
#endif //NUM_NODES
#ifndef CONNECT_TO_WIFI_AP
#define CONNECT_TO_WIFI_AP 1 ///< @brief In projects where gateway should not be connected to WiFi (for instance a data logger to SD) it may be useful to disable WiFi setting this to 0. Set it to 1 otherwise
#endif //CONNECT_TO_WIFI_AP

// Node configuration
static const int16_t RECONNECTION_PERIOD = 1500; ///< @brief Time to retry Gateway connection
static const uint16_t DOWNLINK_WAIT_TIME = 350; ///< @brief Time to wait for downlink message before sleep. Setting less than 180 ms causes ESP-NOW errors due to lack of ACK processing
static const uint32_t DEFAULT_SLEEP_TIME = 10; ///< @brief Default sleep time if it was not set
static const time_t IDENTIFY_TIMEOUT = 10000; ///< @brief How long LED will be flashing during identification
#ifndef TIME_SYNC_PERIOD
static const uint32_t TIME_SYNC_PERIOD = 30000; ///< @brief Period of clock synchronization request
#endif // TIME_SYNC_PERIOD
static const unsigned int QUICK_SYNC_TIME = 5000; ///< @brief Period of clock synchronization request in case of resync is needed 
#ifndef PRE_REG_DELAY
static const uint32_t PRE_REG_DELAY = 5000; ///< @brief Time to wait before registration so that other nodes have time to communicate. Real delay is a random lower than this value.
#endif // PRE_REG_DELAY
static const uint32_t POST_REG_DELAY = 1500; ///< @brief Time to wait before sending data after registration so that other nodes have time to finish their registration. Real delay is a random lower than this value.
static const uint8_t COMM_ERRORS_BEFORE_SCAN = 2; ///< @brief Node will search for a gateway if this number of communication errors have happened.

//Web API
#define ENABLE_WEB_API 1 ///< @brief Enable Web API support on gateway

//Debug
#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial ///< @brief Stream to output debug info. It will normally be `Serial`
#endif // DEBUG_ESP_PORT
#ifndef DEBUG_LEVEL
// DON'T ENABLE DEBUG IF YOU CAN ONLY DO OTA UPDATE. YOU MAY BE UNABLE TO DO OTA UPDATE ANYMORE UNTIL YOU FLASH THE NODE THROUGH WIRE
#define DEBUG_LEVEL WARN ///< @brief Possible values VERBOSE, DBG, INFO, WARN, ERROR, NONE
#endif //DEBUG_LEVEL

#endif
