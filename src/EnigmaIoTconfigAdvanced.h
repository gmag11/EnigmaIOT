/**
  * @file EnigmaIoTconfigAdvanced.h
  * @version 0.9.7
  * @date 04/02/2021
  * @author German Martin
  * @brief Parameter configuration
  */

 /****************************************************************************
  DO NOT MODIFY THESE SETTINGS UNLESS YOU KNOW WHAT YOU ARE DOING
  ---------------------------------------------------------------
  NO MODIFIQUES ESTOS AJUSTES SI NO SABES LO QUE ESTÁS HACIENDO
  ****************************************************************************/

#ifndef _CONFIG_ADVANCED_h
#define _CONFIG_ADVANCED_h

#include "Arduino.h"

// Global configuration. Physical layer settings
static const uint8_t MAX_MESSAGE_LENGTH = 250; ///< @brief Maximum payload size on ESP-NOW
static const uint8_t MAX_DATA_PAYLOAD_LENGTH = 214; ///< @brief Maximum EnigmaIOT user data payload size
static const size_t ENIGMAIOT_ADDR_LEN = 6; ///< @brief Address size. Mac address = 6 bytes
static const uint8_t NETWORK_NAME_LENGTH = 21; ///< @brief Maximum number of characters of network name
static const uint8_t NODE_NAME_LENGTH = 33; ///< @brief Maximum number of characters of node name
static const uint8_t BROADCAST_ADDRESS[] = { 0xff,0xff,0xff,0xff,0xff,0xff }; ///< @brief Broadcast address
static const char BROADCAST_NONE_NAME[] = "broadcast"; ///< @brief Name to reference broadcast node
static const uint8_t COMMS_QUEUE_SIZE = 5;

// Gateway configuration
static const int OTA_GW_TIMEOUT = 11000; ///< @brief OTA mode timeout. In OTA mode all data messages are ignored
#ifndef DISCONNECT_ON_DATA_ERROR
static const bool DISCONNECT_ON_DATA_ERROR = true; ///< @brief Activates node invalidation in case of data error
#endif //DISCONNECT_ON_DATA_ERROR
#ifndef ENABLE_REST_API
#define ENABLE_REST_API 1 ///< @brief Set to 1 to enable REST API
#endif // ENABLE_REST_API
#ifndef SUPPORT_HA_DISCOVERY
#define SUPPORT_HA_DISCOVERY 1  ///< @brief Set to 1 to enable HomeAssistant autodiscovery support
#if SUPPORT_HA_DISCOVERY
static const char HA_DISCOVERY_PREFIX[] = "homeassistant"; ///< @brief Used to build HomeAssistant discovery message topic
#endif // SUPPORT_HA_DISCOVERY
#endif // SUPPORT_HA_DISCOVERY

// Node configuration
static const uint32_t OTA_TIMEOUT_TIME = 10000; ///< @brief Timeout between OTA messages. In milliseconds
static const int MIN_SYNC_ACCURACY = 5000; ///< @brief If calculated offset absolute value is higher than this value resync is done more often. us units
static const int MAX_DATA_PAYLOAD_SIZE = 214; ///< @brief Maximun payload size for data packets
#ifndef CHECK_COMM_ERRORS
static const bool CHECK_COMM_ERRORS = true; ///< @brief Try to reconnect in case of communication errors
#endif // CHECK_COMM_ERRORS
static const uint32_t RTC_ADDRESS = 8; ///< @brief RTC memory address where to store context. Modify it if you need place to store your own data during deep sleep. Take care not to overwrite above that address. It is 8 to give space for FailSafeMode library
#ifndef USE_FLASH_INSTEAD_RTC
#define USE_FLASH_INSTEAD_RTC 0 ///< @brief Use flash instead RTC for temporary context data. ATTENTION: This allows connection to survive power off cycles but may damage flash memory persistently.
#endif // USE_FLASH_INSTEAD_RTC
#ifndef HA_FIRST_DISCOVERY_DELAY
#define HA_FIRST_DISCOVERY_DELAY 5000
#endif // HA_FIRST_DISCOVERY_DELAY
#ifndef HA_NEXT_DISCOVERY_DELAY
#define HA_NEXT_DISCOVERY_DELAY 500
#endif // HA_NEXT_DISCOVERY_DELAY
#ifndef HA_FIRST_DISCOVERY_DELAY_SLEEPY
#define HA_FIRST_DISCOVERY_DELAY_SLEEPY 10
#endif // HA_FIRST_DISCOVERY_DELAY
#ifndef HA_NEXT_DISCOVERY_DELAY_SLEEPY
#define HA_NEXT_DISCOVERY_DELAY_SLEEPY 10
#endif // HA_NEXT_DISCOVERY_DELAY

//Crypto configuration
const uint8_t KEY_LENGTH = 32; ///< @brief Key length used by selected crypto algorythm. The only tested value is 32. Change it only if you know what you are doing
const uint8_t IV_LENGTH = 12; ///< @brief Initalization vector length used by selected crypto algorythm
const uint8_t TAG_LENGTH = 16; ///< @brief Authentication tag length. For Poly1305 it is always 16
const uint8_t AAD_LENGTH = 8; ///< @brief Number of bytes from last part of key that will be used for additional authenticated data
#define CYPHER_TYPE ChaChaPoly

//Web API
const int WEB_API_PORT = 80; ///< @brief TCP port where Web API will listen through

//File system
#if defined ESP32
#define FILESYSTEM SPIFFS
#include <SPIFFS.h>
#elif defined ESP8266
    #ifndef USE_LITTLE_FS
    #define USE_LITTLE_FS 1 ///< Set to 0 to use SPIFFS or 1 to use LittleFS (recommended)
    #endif // USE_LITTLE_FS
#if USE_LITTLE_FS
#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#else
#define FILESYSTEM SPIFFS
#endif // USE_LITTLE_FS
#endif

#endif
