// GwOutput_mqtt.h

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

constexpr auto CONFIG_FILE = "/mqtt.json";

typedef struct {
	char mqtt_server[41];
#ifdef SECURE_MQTT
	int mqtt_port = 8883;
#else
	int mqtt_port = 1883;
#endif //SECURE_MQTT
	char mqtt_user[21];
	char mqtt_pass[41];
} mqttgw_config_t;


class GwOutput_MQTT: public GatewayOutput_generic {
 protected:
	 AsyncWiFiManagerParameter* mqttServerParam;
	 AsyncWiFiManagerParameter* mqttPortParam;
	 AsyncWiFiManagerParameter* mqttUserParam;
	 AsyncWiFiManagerParameter* mqttPassParam;

#ifdef ESP32
	 esp_mqtt_client_config_t mqtt_cfg;
	 esp_mqtt_client_handle_t client;
#endif // ESP32

	 mqttgw_config_t mqttgw_config;
	 bool shouldSaveConfig = false;

#ifdef ESP8266
#ifdef SECURE_MQTT
	 BearSSL::X509List certificate;
	 WiFiClientSecure secureClient;
	 PubSubClient client;
#else
	 WiFiClient unsecureClient;
	 PubSubClient client;
#endif // SECURE_MQTT
#endif // ESP8266

	 bool saveConfig ();
#ifdef ESP32
	 static esp_err_t mqtt_event_handler (esp_mqtt_event_handle_t event);
#endif // ESP32
#ifdef SECURE_MQTT
	 void setClock ();
#endif // SECURE_MQTT
#ifdef ESP8266
	 void reconnect ();
#endif
	 static bool publishMQTT (GwOutput_MQTT* gw, const char* topic, char* payload, size_t len, bool retain = false);
	 static void onDlData (char* topic, uint8_t* data, unsigned int len);

 public:
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
	 void configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw);
	 void configManagerExit (bool status);
	 bool begin ();
	 bool loadConfig ();

	 /**
	  * @brief Send control data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	 bool outputControlSend (char* address, uint8_t *data, uint8_t length);
	 bool newNodeSend (char *address);
	 bool nodeDisconnectedSend (char* address, gwInvalidateReason_t reason);
	 bool outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type = data);
	 void loop ();
};

extern GwOutput_MQTT GwOutput;

#endif

