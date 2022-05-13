#ifndef _RTC_DATA_h
#define _RTC_DATA_h

#include "EnigmaIoTconfigAdvanced.h"
#include "NodeList.h"

/**
  * @brief Context data to be stored con persistent storage to be used after wake from sleep mode
  */
typedef struct {
	uint32_t crc32; /**< CRC to check RTC data integrity */
	uint8_t nodeKey[KEY_LENGTH]; /**< Node shared key */
	uint16_t nodeId; /**< Node identification */
	uint8_t channel /*= DEFAULT_CHANNEL*/; /**< WiFi channel used on ESP-NOW communication */
	uint8_t gateway[ENIGMAIOT_ADDR_LEN]; /**< Gateway address */
	int8_t rssi; /**< Gateway signal strength */
	uint8_t networkKey[KEY_LENGTH]; /**< Network key to protect key agreement */
	char networkName[NETWORK_NAME_LENGTH]; /**< Network name. Used to search gateway peer */
	bool sleepy; /**< Sleepy node */
	uint32_t sleepTime /*= 0*/; /**< Time to sleep between sensor data delivery */
	char nodeName[NODE_NAME_LENGTH + 1]; /**< Node name. Use as a human friendly name to avoid use of numeric address*/
	uint8_t commErrors /*= 0*/; /**< number of non acknowledged packets. May mean that gateway is not available or its channel has changed.
								This is used to retrigger Gateway scan*/
	bool nodeKeyValid /* = false*/; /**< true if key has been negotiated successfully */
	uint8_t broadcastKey[KEY_LENGTH]; /**< Key to encrypt broadcast messages */
	bool broadcastKeyValid /* = false*/; /**< true if broadcast key has been received from gateway */
	bool broadcastKeyRequested /* = false*/; /**< true if broadcast key has been requested to gateway */
	status_t nodeRegisterStatus /*= UNREGISTERED*/; /**< Node registration status */
	uint16_t lastMessageCounter; /**< Node last message counter */
	uint16_t lastControlCounter; /**< Control message last counter */
	uint16_t lastDownlinkMsgCounter; /**< Downlink message last counter */
} rtcmem_data_t;

#endif // _RTC_DATA_h