/**
  * @file GwOutput_mqtt.h
  * @version 0.7.0
  * @date 31/12/2019
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

#include "GwOutput_generic.h"
#include "dstrootca.h"
#include <ESPAsyncWiFiManager.h>
#include <EnigmaIOTGateway.h>

#ifdef ESP32
#include "mqtt_client.h"
#elif defined(ESP8266)
#include <PubSubClient.h>
#else
#error "Platform is not ESP32 or ESP8266"
#endif

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
#define SET_USER_DATA    "set/data"
#define GET_USER_DATA    "get/data"
#define NODE_DATA        "data"
#define LOST_MESSAGES    "debug/lostmessages"
#define NODE_STATUS      "status"
#define GW_STATUS        "/gateway/status"

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


class GwOutput_MQTT: public GatewayOutput_generic {
 protected:
	 AsyncWiFiManagerParameter* mqttServerParam; ///< @brief Configuration field for MQTT server address
	 AsyncWiFiManagerParameter* mqttPortParam; ///< @brief Configuration field for MQTT server port
	 AsyncWiFiManagerParameter* mqttUserParam; ///< @brief Configuration field for MQTT server user name
	 AsyncWiFiManagerParameter* mqttPassParam; ///< @brief Configuration field for MQTT server password

#ifdef ESP32
	 esp_mqtt_client_config_t mqtt_cfg; ///< @brief MQTT server configuration structure
	 esp_mqtt_client_handle_t client; ///< @brief MQTT client handle
#endif // ESP32

	 mqttgw_config_t mqttgw_config; ///< @brief MQTT server configuration data
	 bool shouldSaveConfig = false; ///< @brief Flag to indicate if configuration should be saved

#ifdef ESP8266
#ifdef SECURE_MQTT
	 BearSSL::X509List certificate; ///< @brief CA certificate for TLS
	 WiFiClientSecure secureClient; ///< @brief TLS client
#else
	 WiFiClient unsecureClient; ///< @brief TCP client
#endif // SECURE_MQTT
	 PubSubClient client; ///< @brief MQTT client
#endif // ESP8266

	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	 bool saveConfig ();
#ifdef ESP32
	/**
	  * @brief Manages incoming MQTT events
	  * @param event Event type
	  * @return Error code
	  */
	 static esp_err_t mqtt_event_handler (esp_mqtt_event_handle_t event);
#endif // ESP32
#ifdef SECURE_MQTT
	/**
	  * @brief Synchronizes time over NTP to check certifitate expiration time
	  */
	 void setClock ();
#endif // SECURE_MQTT
#ifdef ESP8266
	/**
	  * @brief Called when wifi manager starts config portal
	  * @param enigmaIotGw Pointer to EnigmaIOT gateway instance
	  */
	 void reconnect ();
#endif

	 /**
	  * @brief Publishes data over MQTT
	  * @param gw MQTT output gateway module instance
	  * @param topic Topic that indicates message type
	  * @param payload Message payload data
	  * @param len Payload length
	  * @param retain `true` if message should be retained
	  */
	 static bool publishMQTT (GwOutput_MQTT* gw, const char* topic, char* payload, size_t len, bool retain = false);

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
	 GwOutput_MQTT ()
#ifdef ESP8266
		 :
#ifdef SECURE_MQTT
		 certificate (DSTroot_CA),
		 client (secureClient)
#else
		 client (unsecureClient)
#endif // SECURE_MQTT
#endif // ESP8266
	 {}

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
	bool outputControlSend (char* address, uint8_t *data, uint8_t length);

	 /**
	  * @brief Send new node notification
	  * @param address Node Address
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool newNodeSend (char *address);

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
	bool outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type = data);

	 /**
	  * @brief Should be called often for module management
	  */
	void loop ();
};

extern GwOutput_MQTT GwOutput;

#endif

