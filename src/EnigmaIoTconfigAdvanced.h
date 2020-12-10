/**
  * @file EnigmaIoTconfigAdvanced.h
  * @version 0.9.6
  * @date 10/12/2020
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

// Global configuration. Physical layer settings
static const uint8_t MAX_MESSAGE_LENGTH = 250; ///< @brief Maximum payload size on ESP-NOW
static const size_t ENIGMAIOT_ADDR_LEN = 6; ///< @brief Address size. Mac address = 6 bytes
static const uint8_t NETWORK_NAME_LENGTH = 21; ///< @brief Maximum number of characters of network name
static const uint8_t NODE_NAME_LENGTH = 33; ///< @brief Maximum number of characters of node name
static const uint8_t BROADCAST_ADDRESS[] = { 0xff,0xff,0xff,0xff,0xff,0xff }; ///< @brief Broadcast address
static const char BROADCAST_NONE_NAME[] = "broadcast"; ///< @brief Name to reference broadcast node

// Gateway configuration
static const int OTA_GW_TIMEOUT = 11000; ///< @brief OTA mode timeout. In OTA mode all data messages are ignored
#ifndef DISCONNECT_ON_DATA_ERROR
static const bool DISCONNECT_ON_DATA_ERROR = true; ///< @brief Activates node invalidation in case of data error
#endif //DISCONNECT_ON_DATA_ERROR

// Node configuration
static const uint32_t OTA_TIMEOUT_TIME = 10000; ///< @brief Timeout between OTA messages. In milliseconds
static const int MIN_SYNC_ACCURACY = 5; ///< @brief If calculated offset absolute value is higher than this value resync is done more often
static const int MAX_DATA_PAYLOAD_SIZE = 214; ///< @brief Maximun payload size for data packets
#ifndef CHECK_COMM_ERRORS
static const bool CHECK_COMM_ERRORS = true; ///< @brief Try to reconnect in case of communication errors
#endif // !1
static const uint32_t RTC_ADDRESS = 8; ///< @brief RTC memory address where to store context. Modify it if you need place to store your own data during deep sleep. Take care not to overwrite above that address. It is 8 to give space for FailSafeMode library
#define USE_FLASH_INSTEAD_RTC 0 ///< @brief Use flash instead RTC for temporary context data. ATTENTION: This allows connection to survive power off cycles but may damage flash memory persistently.

//Crypto configuration
const uint8_t KEY_LENGTH = 32; ///< @brief Key length used by selected crypto algorythm. The only tested value is 32. Change it only if you know what you are doing
const uint8_t IV_LENGTH = 12; ///< @brief Initalization vector length used by selected crypto algorythm
const uint8_t TAG_LENGTH = 16; ///< @brief Authentication tag length. For Poly1305 it is always 16
const uint8_t AAD_LENGTH = 8; ///< @brief Number of bytes from last part of key that will be used for additional authenticated data
#define CYPHER_TYPE ChaChaPoly

//Web API
const int WEB_API_PORT = 80; ///< @brief TCP port where Web API will listen through

#endif
