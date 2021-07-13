/**
  * @file GwOutput_mqtt.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief MQTT Gateway output module
  *
  * Module to send and receive EnigmaIOT information from MQTT broker
  */

#ifndef _GWOUTPUT_MQTT_h
#define _GWOUTPUT_MQTT_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <GwOutput_generic.h>
#include <ESPAsyncWiFiManager.h>
#include <EnigmaIOTGateway.h>
#include <PubSubClient.h>
#include <queue>
#include "dstrootca.h"
#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif // SECURE_MQTT

#define MQTT_BUFFER_SIZE 2048

// DOWNLINK MESSAGES
#define GET_VERSION      "get/version"
#define GET_VERSION_ANS  "result/version"
#define GET_SLEEP        "get/sleeptime"
#define GET_SLEEP_ANS    "result/sleeptime"
#define SET_SLEEP        "set/sleeptime"
#define SET_OTA          "set/ota"
#define SET_OTA_ANS      "result/ota"
#define SET_IDENTIFY     "set/identify"
#define SET_RESET_CONFIG "set/reset"
#define SET_RESET_ANS    "result/reset"
#define GET_RSSI         "get/rssi"
#define GET_RSSI_ANS     "result/rssi"
#define GET_NAME         "get/name"
#define SET_NAME         "set/name"
#define GET_NAME_ANS     "result/name"
#define SET_USER_DATA    "set/data"
#define GET_USER_DATA    "get/data"
#define RESTART_NOTIF    "restart"
#define NODE_DATA        "data"
#define LOST_MESSAGES    "debug/lostmessages"
#define NODE_STATUS      "status"
#define GW_STATUS        "/gateway/status"
#define SET_RESTART_MCU	 "set/restart"

const time_t STATUS_SEND_PERIOD = 300000;

constexpr auto CONFIG_FILE = "/mqtt.json"; ///< @brief MQTT outout configuration file name

typedef struct {
	char mqtt_server[41]; /**< MQTT broker address*/
#ifdef SECURE_MQTT
	int mqtt_port = 8883; /**< MQTT broker TCP port*/
#else
	int mqtt_port = 1883; /**< MQTT broker TCP port*/
#endif //SECURE_MQTT
	char mqtt_user[21]; /**< MQTT broker user name*/
	char mqtt_pass[41]; /**< MQTT broker user password*/
} mqttgw_config_t;

constexpr auto MAX_MQTT_TOPIC_LEN = 100;
constexpr auto MAX_MQTT_PLD_LEN = 2048;

typedef struct {
	char topic[MAX_MQTT_TOPIC_LEN]; /**< Message topic*/
	char payload[MAX_MQTT_PLD_LEN]; /**< Message payload*/
	size_t payload_len; /**< Payload length*/
	bool retain; /**< MQTT retain flag*/
} mqtt_queue_item_t;


class GwOutput_MQTT : public GatewayOutput_generic {
protected:
	AsyncWiFiManagerParameter* mqttServerParam = NULL; ///< @brief Configuration field for MQTT server address
    AsyncWiFiManagerParameter* mqttPortParam = NULL; ///< @brief Configuration field for MQTT server port
    AsyncWiFiManagerParameter* mqttUserParam = NULL; ///< @brief Configuration field for MQTT server user name
    AsyncWiFiManagerParameter* mqttPassParam = NULL; ///< @brief Configuration field for MQTT server password

	//std::queue<mqtt_queue_item_t*> mqtt_queue; ///< @brief Output MQTT messages queue. It acts as a FIFO queue
    EnigmaIOTRingBuffer<mqtt_queue_item_t> mqtt_queue;

	mqttgw_config_t mqttgw_config; ///< @brief MQTT server configuration data
	bool shouldSaveConfig = false; ///< @brief Flag to indicate if configuration should be saved

#ifdef SECURE_MQTT
	WiFiClientSecure espClient; ///< @brief TLS client
#ifdef ESP8266
	BearSSL::X509List certificate; ///< @brief CA certificate for TLS
#endif // ESP8266
#else
	WiFiClient espClient; ///< @brief TCP client
#endif // SECURE_MQTT
	PubSubClient mqtt_client; ///< @brief MQTT client
    
   /**
	 * @brief Saves output module configuration
	 * @return Returns `true` if save was successful. `false` otherwise
	 */
	bool saveConfig ();
//#ifdef SECURE_MQTT
	/**
	  * @brief Synchronizes time over NTP to check certifitate expiration time
	  */
	void setClock ();
//#endif // SECURE_MQTT
	/**
	  * @brief This is called anytime MQTT client is disconnected.
	  *
	  * It tries to connect to MQTT broker. After reconnection is done it resubscribes
	  * to network topics.
	  * It waits for connection and times out after 5 seconds
	  */
	void reconnect ();

   /**
	 * @brief Add MQTT message to queue
	 * @param topic MQTT message topic
	 * @param payload MQTT message payload
	 * @param len MQTT payload length
	 * @param retain Message retain flag
	 */
	bool addMQTTqueue (const char* topic, char* payload, size_t len, bool retain = false);

   /**
	 * @brief Gets next item in the queue
	 * @return Next MQTT message to be sent
	 */
	mqtt_queue_item_t* getMQTTqueue ();

   /**
	 * @brief Deletes next item in the queue
	 */
	void popMQTTqueue ();

	/**
	 * @brief Publishes data over MQTT
	 * @param topic Topic that indicates message type
	 * @param payload Message payload data
	 * @param len Payload length
	 * @param retain `true` if message should be retained
	 */
	bool publishMQTT (const char* topic, const char* payload, size_t len, bool retain = false);

   /**
	 * @brief Function that processes downlink data from network to node
	 * @param topic Topic that indicates message type
	 * @param data Message payload
	 * @param len Payload length
	 */
	static void onDlData (char* topic, uint8_t* data, unsigned int len);

public:
   /**
	 * @brief Constructor to initialize MQTT client
	 */
	GwOutput_MQTT () :
        mqtt_queue (MAX_MQTT_QUEUE_SIZE),
#if defined ESP8266 && defined SECURE_MQTT
		certificate (DSTroot_CA),
#endif // ESP8266 && SECURE_MQTT
		mqtt_client (espClient) {}

	/**
	  * @brief Called when wifi manager starts config portal
	  * @param enigmaIotGw Pointer to EnigmaIOT gateway instance
	  */
	void configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw);

	/**
	  * @brief Called when wifi manager exits config portal
	  * @param status `true` if configuration was successful
	  */
	void configManagerExit (bool status);

	/**
	  * @brief Starts output module
	  * @return Returns `true` if successful. `false` otherwise
	  */
	bool begin ();

	/**
	  * @brief Loads output module configuration
	  * @return Returns `true` if load was successful. `false` otherwise
	  */
	bool loadConfig ();

	/**
	  * @brief Send control data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool outputControlSend (char* address, uint8_t* data, size_t length);

	 /**
	  * @brief Send new node notification
	  * @param address Node Address
	  * @param node_id Node Id
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool newNodeSend (char* address, uint16_t node_id);

	 /**
	  * @brief Send node disconnection notification
	  * @param address Node Address
	  * @param reason Disconnection reason code
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool nodeDisconnectedSend (char* address, gwInvalidateReason_t reason);

	 /**
	  * @brief Send data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @param type Type of message
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
    bool outputDataSend (char* address, char* data, size_t length, GwOutput_data_type_t type = data);

#if SUPPORT_HA_DISCOVERY
    /**
      * @brief Send MQTT generic message
      * @param topic MQTT topic
      * @param payload Message data buffer
      * @param len Data buffer length
      * @param retain MQTT retain parameter
      * @return Returns `true` if sending was successful. `false` otherwise
      */
    bool rawMsgSend (const char* topic, char* payload, size_t len, bool retain);
#endif
    
	 /**
	  * @brief Should be called regularly for module management
	  */
	void loop ();
};

extern GwOutput_MQTT GwOutput;

#endif // _GWOUTPUT_MQTT_h

