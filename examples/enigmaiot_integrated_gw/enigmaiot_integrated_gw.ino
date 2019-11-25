/**
  * @file enigmaiot_gateway.ino
  * @version 0.6.0
  * @date 17/11/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  *
  * Communicates with a serial to MQTT gateway to send data to any IoT platform
  */

#ifdef ESP32
#include "GwOutput_mqtt.h"
#include <WiFi.h> // Comment to compile for ESP8266
#include <AsyncTCP.h> // Comment to compile for ESP8266
#include <Update.h>
#include <SPIFFS.h>
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
//#include <ESPAsyncTCP.h> // Comment to compile for ESP32
#include <Hash.h>
#include <SPI.h>
#include <PubSubClient.h>
#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif // SECURE_MQTT
#endif // ESP32


#include <Arduino.h>
#include <CayenneLPP.h>
#include <FS.h>

#include "dstrootca.h"

#include "EnigmaIOTGateway.h"
#include "lib/helperFunctions.h"
#include "lib/debug.h"
#include "espnow_hal.h"

#include <Curve25519.h>
#include <ChaChaPoly.h>
#include <Poly1305.h>
#include <SHA256.h>
#include <CRC32.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include "GwOutput_generic.h"

#ifndef BUILTIN_LED
#define BUILTIN_LED 5
#endif // BUILTIN_LED

#define BLUE_LED BUILTIN_LED
#define RED_LED BUILTIN_LED

#ifdef ESP32
TimerHandle_t connectionLedTimer;
#elif defined(ESP8266)
ETSTimer connectionLedTimer;
#endif // ESP32

const int connectionLed = BUILTIN_LED;
boolean connectionLedFlashing = false;


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

#ifdef ESP32
esp_mqtt_client_config_t mqtt_cfg;
esp_mqtt_client_handle_t client;
#endif // ESP32

mqttgw_config_t mqttgw_config;
const char CONFIG_FILE[] = "/mqtt.conf";
bool shouldSaveConfig = false;

#ifdef ESP8266
#ifdef SECURE_MQTT
BearSSL::X509List certificate (DSTroot_CA);
WiFiClientSecure secureClient;
PubSubClient client (secureClient);
#else
WiFiClient unsecureClient;
PubSubClient client (unsecureClient);
#endif // SECURE_MQTT
#endif // ESP8266


AsyncWiFiManagerParameter *mqttServerParam;
AsyncWiFiManagerParameter *mqttPortParam;
AsyncWiFiManagerParameter *mqttUserParam;
AsyncWiFiManagerParameter *mqttPassParam;

String netName;
String clientId;
String gwTopic;

void publishMQTT (const char* topic, char* payload, size_t len, bool retain = false);

void flashConnectionLed (void* led) {
	//digitalWrite (*(int*)led, !digitalRead (*(int*)led));
	digitalWrite (BUILTIN_LED, !digitalRead (BUILTIN_LED));
}

void startConnectionFlash (int period) {
#ifdef ESP32
	if (!connectionLedFlashing) {
		connectionLedFlashing = true;
		connectionLedTimer = xTimerCreate ("led_flash", pdMS_TO_TICKS (period), pdTRUE, (void*)0, flashConnectionLed);
		xTimerStart (connectionLedTimer, 0);
	}
#elif defined (ESP8266)
	ets_timer_disarm (&connectionLedTimer);
	if (!connectionLedFlashing) {
		connectionLedFlashing = true;
		ets_timer_arm_new (&connectionLedTimer, period, true, true);
	}
#endif // ESP32
}

void stopConnectionFlash () {
#ifdef ESP32
	if (connectionLedFlashing) {
		connectionLedFlashing = false;
		xTimerStop (connectionLedTimer, 0);
		xTimerDelete (connectionLedTimer, 0);
	}
#elif defined(ESP8266)
	if (connectionLedFlashing) {
		connectionLedFlashing = false;
		ets_timer_disarm (&connectionLedTimer);
		digitalWrite (connectionLed, HIGH);
	}
#endif // ESP32
}

bool saveMQTTConfig () {
	if (!SPIFFS.begin ()) {
		DEBUG_WARN ("Error opening filesystem");
	}
	DEBUG_DBG ("Filesystem opened");

	File configFile = SPIFFS.open (CONFIG_FILE, "w");
	if (!configFile) {
		DEBUG_WARN ("Failed to open config file %s for writing", CONFIG_FILE);
		return false;
	} else {
		DEBUG_DBG ("%s opened for writting", CONFIG_FILE);
	}
	configFile.write ((uint8_t*)(&mqttgw_config), sizeof (mqttgw_config));
	configFile.close ();
	DEBUG_DBG ("Gateway configuration saved to flash");
	return true;
}

bool loadBridgeConfig () {
	//SPIFFS.remove (CONFIG_FILE); // Only for testing

	if (!SPIFFS.begin ()) {
		DEBUG_WARN ("Error starting filesystem. Formatting");
		SPIFFS.format ();
		WiFi.disconnect ();
	}

	if (SPIFFS.exists (CONFIG_FILE)) {
		DEBUG_DBG ("Opening %s file", CONFIG_FILE);
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
			DEBUG_DBG ("%s opened", CONFIG_FILE);
			size_t size = configFile.size ();
			if (size < sizeof (mqttgw_config_t)) {
				DEBUG_WARN ("Config file is corrupted. Deleting");
				SPIFFS.remove (CONFIG_FILE);
				return false;
			}
			configFile.read ((uint8_t*)(&mqttgw_config), sizeof (mqttgw_config_t));
			configFile.close ();
			DEBUG_INFO ("Gateway configuration successfuly read");
			DEBUG_DBG ("MQTT server: %s", mqttgw_config.mqtt_server);
			DEBUG_DBG ("MQTT port: %d", mqttgw_config.mqtt_port);
			DEBUG_DBG ("MQTT user: %s", mqttgw_config.mqtt_user);
			return true;
		}
	} else {
		DEBUG_WARN ("%s do not exist", CONFIG_FILE);
		return false;
	}

	return false;
}

void wifiManagerExit (boolean status) {
	DEBUG_INFO ("==== Config Portal MQTTGW result ====");
	DEBUG_INFO ("MQTT server: %s", mqttServerParam->getValue ());
	DEBUG_INFO ("MQTT port: %s", mqttPortParam->getValue ());
	DEBUG_INFO ("MQTT user: %s", mqttUserParam->getValue ());
	DEBUG_INFO ("MQTT password: %s", mqttPassParam->getValue ());
	DEBUG_INFO ("Status: %s", status ? "true" : "false");

	if (status && EnigmaIOTGateway.getShouldSave()) {
		memcpy (mqttgw_config.mqtt_server, mqttServerParam->getValue (), mqttServerParam->getValueLength ());
		mqttgw_config.mqtt_server[mqttServerParam->getValueLength ()] = '\0';
		DEBUG_DBG ("MQTT Server: %s", mqttgw_config.mqtt_server);
		mqttgw_config.mqtt_port = atoi (mqttPortParam->getValue ());
		memcpy (mqttgw_config.mqtt_user, mqttUserParam->getValue (), mqttUserParam->getValueLength ());
		const char* mqtt_pass = mqttPassParam->getValue ();
		if (mqtt_pass && (mqtt_pass[0] != '\0')) {// If password is empty, keep the old one
			memcpy (mqttgw_config.mqtt_pass, mqtt_pass, mqttPassParam->getValueLength ());
			mqttgw_config.mqtt_pass[mqttPassParam->getValueLength ()] = '\0';
		} else {
			DEBUG_INFO ("MQTT password field empty. Keeping the old one");
		}
		DEBUG_DBG ("MQTT pass: %s", mqttgw_config.mqtt_pass);
		if (!saveMQTTConfig()) {
			DEBUG_ERROR ("Error writting MQTT config to filesystem.");
		} else {
			DEBUG_INFO ("Configuration stored");
		}
	} else {
		DEBUG_DBG ("Configuration does not need to be saved");
	}

	free (mqttServerParam);
	free (mqttPortParam);
	free (mqttUserParam);
	free (mqttPassParam);
}

void wifiManagerStarted () {

	//configManagerStart (EnigmaIOTGatewayClass configManager);

	mqttServerParam = new AsyncWiFiManagerParameter ("mqttserver", "MQTT Server", mqttgw_config.mqtt_server, 41, "required type=\"text\" maxlength=40");
	char port[10];
	itoa (mqttgw_config.mqtt_port, port, 10);
	mqttPortParam = new AsyncWiFiManagerParameter ("mqttport", "MQTT Port", port, 6, "required type=\"number\" min=\"0\" max=\"65535\" step=\"1\"");
	mqttUserParam = new AsyncWiFiManagerParameter ("mqttuser", "MQTT User", mqttgw_config.mqtt_user, 21, "required type=\"text\" maxlength=20");
	mqttPassParam = new AsyncWiFiManagerParameter ("mqttpass", "MQTT Password", "", 41, "type=\"password\" maxlength=40");

	EnigmaIOTGateway.addWiFiManagerParameter (mqttServerParam);
	EnigmaIOTGateway.addWiFiManagerParameter (mqttPortParam);
	EnigmaIOTGateway.addWiFiManagerParameter (mqttUserParam);
	EnigmaIOTGateway.addWiFiManagerParameter (mqttPassParam);

}

void onDlData (const char* topic, char* payload, unsigned int length) {}

void processRxControlData (char* macStr, const uint8_t* data, uint8_t length) {
	const int TOPIC_SIZE = 64;
	const int PAYLOAD_SIZE = 512;
	char* topic = (char*)malloc (TOPIC_SIZE);
	char* payload = (char*)malloc (PAYLOAD_SIZE);
	size_t pld_size;

	switch (data[0]) {
		case control_message_type::VERSION_ANS:
			Serial.printf ("~/%s/%s;{\"version\":\"", macStr, GET_VERSION_ANS);
			Serial.write (data + 1, length - 1);
			Serial.println ("\"}");
			Serial.printf ("%s/%s/%s;{\"version\":\"", EnigmaIOTGateway.getNetworkName (), macStr, GET_VERSION_ANS);
			break;
		case control_message_type::SLEEP_ANS:
			uint32_t sleepTime;
			memcpy (&sleepTime, data + 1, sizeof (sleepTime));
			Serial.printf ("~/%s/%s;{\"sleeptime\":%d}\n", macStr, GET_SLEEP_ANS, sleepTime);
			break;
		case control_message_type::RESET_ANS:
			Serial.printf ("~/%s/%s;{}\n", macStr, SET_RESET_ANS);
			break;
		case control_message_type::RSSI_ANS:
			//Serial.printf ("~/%s/%s;{\"rssi\":%d,\"channel\":%u}\n", macStr, GET_RSSI_ANS, (int8_t)data[1], data[2]);
			snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName, macStr, GET_RSSI_ANS);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"rssi\":%d,\"channel\":%u}", (int8_t)data[1], data[2]);
			publishMQTT (topic, payload, pld_size);
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			break;
		case control_message_type::OTA_ANS:
			Serial.printf ("~/%s/%s;", macStr, SET_OTA_ANS);
			switch (data[1]) {
				case ota_status::OTA_STARTED:
					Serial.printf ("{\"result\":\"OTA Started\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_START_ERROR:
					Serial.printf ("{\"result\":\"OTA Start error\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_OUT_OF_SEQUENCE:
					uint16_t lastGoodIdx;
					memcpy ((uint8_t*)& lastGoodIdx, data + 2, sizeof (uint16_t));
					Serial.printf ("{\"last_chunk\":%d,\"result\":\"OTA out of sequence error\",\"status\":%u}\n", lastGoodIdx, data[1]);
					break;
				case ota_status::OTA_CHECK_OK:
					Serial.printf ("{\"result\":\"OTA check OK\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_CHECK_FAIL:
					Serial.printf ("{\"result\":\"OTA check failed\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_TIMEOUT:
					Serial.printf ("{\"result\":\"OTA timeout\",\"status\":%u}\n", data[1]);
					break;
				case ota_status::OTA_FINISHED:
					Serial.printf ("{\"result\":\"OTA finished OK\",\"status\":%u}\n", data[1]);
					break;
				default:
					Serial.println ();
			}
			break;
	}
}

void publishMQTT (const char* topic, char* payload, size_t len, bool retain) {
#ifdef ESP32
	esp_mqtt_client_publish (client, topic, payload, len, 0, retain);
#elif defined(ESP8266)
	client.publish (topic, payload, len, retain);
#endif // ESP32
}

void processRxData (uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages, bool control) {
	uint8_t *addr = mac;
	const int capacity = JSON_ARRAY_SIZE (25) + 25 * JSON_OBJECT_SIZE (4);
	DynamicJsonDocument jsonBuffer (capacity);
	//StaticJsonDocument<capacity> jsonBuffer;
	JsonArray root = jsonBuffer.createNestedArray ();
	CayenneLPP *cayennelpp = new CayenneLPP(MAX_DATA_PAYLOAD_SIZE);

	char mac_str[18];
	mac2str (addr, mac_str);
	if (control) {
		processRxControlData (mac_str, buffer, length);
		return;
	}

	char* netName = EnigmaIOTGateway.getNetworkName ();
	const int TOPIC_SIZE = 64;
	const int PAYLOAD_SIZE = 512;
	char* topic = (char*)malloc (TOPIC_SIZE);
	char* payload = (char*)malloc (PAYLOAD_SIZE);

	//Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));

	cayennelpp->decode ((uint8_t *)buffer, length, root);
	cayennelpp->CayenneLPP::~CayenneLPP();
	free (cayennelpp);
	
	//Serial.println ();
	//Serial.printf ("~/%s/data;", mac_str);
	snprintf (topic, TOPIC_SIZE, "%s/%s/data", netName, mac_str);
	size_t pld_size = serializeJson (root, payload, PAYLOAD_SIZE);
	publishMQTT (topic, payload, pld_size);
	DEBUG_INFO ("Published MQTT %s %s", topic, payload);
	if (lostMessages > 0) {
		//serial.printf ("%u lost messages\n", lostmessages);
		//Serial.printf ("~/%s/debug/lostmessages;%u\n", mac_str, lostMessages);
		snprintf (topic, TOPIC_SIZE, "%s/%s/debug/lostmessages", netName, mac_str);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "%u", lostMessages);
		publishMQTT (topic, payload, pld_size);
		DEBUG_INFO ("Published MQTT %s %s", topic, payload);
	}
	//Serial.printf ("~/%s/status;{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%.2f}\n",
	//	mac_str,
	//	EnigmaIOTGateway.getPER ((uint8_t*)mac),
	//	EnigmaIOTGateway.getErrorPackets ((uint8_t*)mac),
	//	EnigmaIOTGateway.getTotalPackets ((uint8_t*)mac),
	//	EnigmaIOTGateway.getPacketsHour ((uint8_t*)mac));
	snprintf (topic, TOPIC_SIZE, "%s/%s/status", netName, mac_str);
	pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%.2f}",
			  EnigmaIOTGateway.getPER ((uint8_t*)mac),
			  EnigmaIOTGateway.getErrorPackets ((uint8_t*)mac),
			  EnigmaIOTGateway.getTotalPackets ((uint8_t*)mac),
			  EnigmaIOTGateway.getPacketsHour ((uint8_t*)mac));
	publishMQTT (topic, payload, pld_size);
	DEBUG_INFO ("Published MQTT %s %s", topic, payload);

	free (payload);
	free (topic);
}

control_message_type_t checkMsgType (String data) {
	if (data.indexOf (GET_VERSION) != -1) {
		return control_message_type::VERSION;
	} else
	if (data.indexOf (GET_SLEEP) != -1) {
		return control_message_type::SLEEP_GET;
	} else
	if (data.indexOf (SET_SLEEP) != -1) {
		return control_message_type::SLEEP_SET;
	} else
	if (data.indexOf (SET_OTA) != -1) {
		return control_message_type::OTA;
	} else
	if (data.indexOf (SET_IDENTIFY) != -1) {
		DEBUG_WARN ("IDENTIFY MESSAGE %s", data.c_str ());
		return control_message_type::IDENTIFY;
	} else
	if (data.indexOf (SET_RESET_CONFIG) != -1) {
		DEBUG_WARN ("RESET CONFIG MESSAGE %s", data.c_str ());
		return control_message_type::RESET;
	}
	if (data.indexOf (GET_RSSI) != -1) {
		DEBUG_INFO ("GET RSSI MESSAGE %s", data.c_str ());
		return control_message_type::RSSI_GET;
	}
	return control_message_type::USERDATA;
}

void onSerial (String message) {
	uint8_t addr[6];

	DEBUG_VERBOSE ("Downlink message: %s", message.c_str ());
	String addressStr = message.substring (message.indexOf ('/') + 1, message.indexOf ('/', 2));
	DEBUG_INFO ("Downlink message to: %s", addressStr.c_str ());
	if (!str2mac (addressStr.c_str (), addr)) {
		DEBUG_ERROR ("Not a mac address");
		return;
	}
	String dataStr = message.substring (message.indexOf ('/', 2) + 1);
	dataStr.trim ();

	control_message_type_t msgType = checkMsgType (dataStr);

	// Add end of string to all control messages
	if (msgType != control_message_type::USERDATA) {
		dataStr = dataStr.substring (dataStr.indexOf (';') + 1);
		dataStr += '\0';
	}

	DEBUG_VERBOSE ("Message %s", dataStr.c_str ());
	DEBUG_INFO ("Message type %d", msgType);
	DEBUG_INFO ("Data length %d", dataStr.length ());
	if (!EnigmaIOTGateway.sendDownstream (addr, (uint8_t*)dataStr.c_str (), dataStr.length (), msgType)) {
		DEBUG_ERROR ("Error sending esp_now message to %s", addressStr.c_str ());
	}
	else {
		DEBUG_DBG ("Esp-now message sent or queued correctly");
	}
}

void newNodeConnected (uint8_t * mac) {
	char macstr[18];
	mac2str (mac, macstr);
	//Serial.printf ("New node connected: %s\n", macstr);

	const int TOPIC_SIZE = 64;
	char* topic = (char*)malloc (TOPIC_SIZE);
	snprintf (topic, TOPIC_SIZE, "%s/%s/hello", netName, macstr);
	publishMQTT (topic, "", 0);
	DEBUG_INFO ("Published MQTT %s", topic);

	//Serial.printf ("~/%s/hello\n", macstr);
}

void nodeDisconnected (uint8_t * mac, gwInvalidateReason_t reason) {
	char macstr[18];
	mac2str (mac, macstr);
	//Serial.printf ("Node %s disconnected. Reason %u\n", macstr, reason);
	Serial.printf ("~/%s/bye;{\"reason\":%u}\n", macstr, reason);
}

#ifdef SECURE_MQTT
void setClock () {
	configTime (1 * 3600, 0, "pool.ntp.org", "time.nist.gov");
#if DEBUG_LEVEL >= INFO
	DEBUG_INFO ("\nWaiting for NTP time sync: ");
	time_t now = time (nullptr);
	while (now < 8 * 3600 * 2) {
		delay (500);
		Serial.print (".");
		now = time (nullptr);
	}
	//Serial.println ("");
	struct tm timeinfo;
	gmtime_r (&now, &timeinfo);
	DEBUG_INFO ("Current time: %s", asctime (&timeinfo));
#endif
}
#endif

#ifdef ESP8266
void reconnect () {
	// Loop until we're reconnected
	while (!client.connected ()) {
		startConnectionFlash (500);
		DEBUG_INFO ("Attempting MQTT connection...");
		// Create a random client ID
		// Attempt to connect
#ifdef SECURE_MQTT
		setClock ();
#endif
		DEBUG_DBG ("Clock set.");
		DEBUG_DBG ("Connect to MQTT server: user %s, pass %s, topic %s",
					mqttgw_config.mqtt_user, mqttgw_config.mqtt_pass, gwTopic.c_str ());
		//client.setServer (mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
		if (client.connect (clientId.c_str (), mqttgw_config.mqtt_user, mqttgw_config.mqtt_pass, gwTopic.c_str (), 0, true, "0", true)) {
			DEBUG_INFO ("connected");
			// Once connected, publish an announcement...
			//String gwTopic = BASE_TOPIC + String("/gateway/hello");
			publishMQTT (gwTopic.c_str (), "1", 1, true);
			// ... and resubscribe
			String dlTopic = netName + String ("/+/set/#");
			client.subscribe (dlTopic.c_str ());
			dlTopic = netName + String ("/+/get/#");
			client.subscribe (dlTopic.c_str ());
			client.setCallback (onDlData);
			stopConnectionFlash ();
		} else {
			client.disconnect ();
			DEBUG_ERROR ("failed, rc=%d try again in 5 seconds", client.state ());
#ifdef SECURE_MQTT
			char error[256];
#ifdef ESP8266
			int errorCode = secureClient.getLastSSLError (error, 256);
#elif defined ESP32
			int errorCode = secureClient.lastError (error, 256);
#endif
			DEBUG_ERROR ("Connect error %d, %s", errorCode, error);
#endif
			// Wait 5 seconds before retrying
			delay (5000);
		}
		//delay (0);
	}
}
#endif

#ifdef ESP32
static esp_err_t mqtt_event_handler (esp_mqtt_event_handle_t event) {
	if (event->event_id == MQTT_EVENT_CONNECTED) {
		ESP_LOGI ("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_CONNECTED", event->msg_id, event->event_id);
		esp_mqtt_client_subscribe (client, "test/hello", 0);
		//esp_mqtt_client_publish (client, "test/status", "1", 1, 0, false);
		publishMQTT (gwTopic.c_str (), "1", 1, true);
	} else if (event->event_id == MQTT_EVENT_DISCONNECTED) {
		ESP_LOGI ("TEST", "MQTT event: %d. MQTT_EVENT_DISCONNECTED", event->event_id);
		//esp_mqtt_client_reconnect (event->client); //not needed if autoconnect is enabled
	} else  if (event->event_id == MQTT_EVENT_SUBSCRIBED) {
		ESP_LOGI ("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_SUBSCRIBED", event->msg_id, event->event_id);
	} else  if (event->event_id == MQTT_EVENT_UNSUBSCRIBED) {
		ESP_LOGI ("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_UNSUBSCRIBED", event->msg_id, event->event_id);
	} else  if (event->event_id == MQTT_EVENT_PUBLISHED) {
		ESP_LOGI ("TEST", "MQTT event: %d. MQTT_EVENT_PUBLISHED", event->event_id);
	} else  if (event->event_id == MQTT_EVENT_DATA) {
		ESP_LOGI ("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_DATA", event->msg_id, event->event_id);
		ESP_LOGI ("TEST", "Topic length %d. Data length %d", event->topic_len, event->data_len);
		ESP_LOGI ("TEST", "Incoming data: %.*s %.*s\n", event->topic_len, event->topic, event->data_len, event->data);
		onDlData (event->topic, event->data, event->data_len);

	} else  if (event->event_id == MQTT_EVENT_BEFORE_CONNECT) {
		ESP_LOGI ("TEST", "MQTT event: %d. MQTT_EVENT_BEFORE_CONNECT", event->event_id);
	}
}

void EnigmaIOTGateway_handle (void * param) {
	for (;;) {
		EnigmaIOTGateway.handle ();
		vTaskDelay (0);
	}
}

TaskHandle_t xEnigmaIOTGateway_handle = NULL;

/*void client_reconnect (void* param) {
	for (;;) {
		vTaskDelay (1);
		if (!client.connected ()) {
			DEBUG_INFO ("reconnect");
			reconnect ();
		}
	}
}

TaskHandle_t xClient_reconnect = NULL;*/
#endif // ESP32

bool initMQTTclient () {
#ifdef SECURE_MQTT
#ifdef ESP32
	esp_err_t err = esp_tls_set_global_ca_store ((const unsigned char*)DSTroot_CA, sizeof (DSTroot_CA));
	ESP_LOGI ("TEST", "CA store set. Error = %d %s", err, esp_err_to_name (err));
	if (err != ESP_OK) {
		return false;
	}
#elif defined(ESP8266)
	randomSeed (micros ());
	secureClient.setTrustAnchors (&certificate);
#endif // ESP32
#endif // SECURE_MQTT
#ifdef ESP8266
	client.setServer (mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
	DEBUG_INFO ("Set MQTT server %s - port %d", mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
#endif

	netName = String (EnigmaIOTGateway.getNetworkName ());

#ifdef ESP32
	uint64_t chipid = ESP.getEfuseMac ();
	clientId = netName + String ((uint32_t)chipid, HEX);
#elif defined(ESP8266)
	clientId = netName + String (ESP.getChipId (), HEX);
#endif // ESP32
	gwTopic = netName + String ("/gateway/status");
#ifdef ESP32
	mqtt_cfg.host = mqttgw_config.mqtt_server;
	mqtt_cfg.port = mqttgw_config.mqtt_port;
	mqtt_cfg.username = mqttgw_config.mqtt_user;
	mqtt_cfg.password = mqttgw_config.mqtt_pass;
	mqtt_cfg.keepalive = 15;
#ifdef SECURE_MQTT
	mqtt_cfg.transport = MQTT_TRANSPORT_OVER_SSL;
#else
	mqtt_cfg.transport = MQTT_TRANSPORT_OVER_TCP;
#endif
	mqtt_cfg.event_handle = mqtt_event_handler;
	mqtt_cfg.lwt_topic = gwTopic.c_str();
	mqtt_cfg.lwt_msg = "0";
	mqtt_cfg.lwt_msg_len = 1;
	mqtt_cfg.lwt_retain = true;

	client = esp_mqtt_client_init (&mqtt_cfg);
	err = esp_mqtt_client_start (client);
	ESP_LOGI ("TEST", "Client connect. Error = %d %s", err, esp_err_to_name (err));
	if (err != ESP_OK)
		return false;
#elif defined(ESP8266)
	reconnect ();
#endif // ESP32
	return true;
}


void setup () {
	Serial.begin (115200); Serial.println (); Serial.println ();
#ifdef ESP8266
	ets_timer_setfn (&connectionLedTimer, flashConnectionLed, (void*)&connectionLed);
#elif defined ESP32
	
#endif
	pinMode (BUILTIN_LED, OUTPUT);
	digitalWrite (BUILTIN_LED, HIGH);
	startConnectionFlash (100);


	if (!loadBridgeConfig ()) {
		DEBUG_WARN ("Error reading config file");
	}

	EnigmaIOTGateway.setRxLed (BLUE_LED);
	EnigmaIOTGateway.setTxLed (RED_LED);
	EnigmaIOTGateway.onNewNode (newNodeConnected);
	EnigmaIOTGateway.onNodeDisconnected (nodeDisconnected);
	EnigmaIOTGateway.onWiFiManagerStarted (wifiManagerStarted);
	EnigmaIOTGateway.onWiFiManagerExit (wifiManagerExit);
	EnigmaIOTGateway.onDataRx (processRxData);
	EnigmaIOTGateway.begin (&Espnow_hal);

	WiFi.mode (WIFI_AP_STA);
	WiFi.begin ();

	EnigmaIOTGateway.configWiFiManager ();

	WiFi.softAP (EnigmaIOTGateway.getNetworkName (), EnigmaIOTGateway.getNetworkKey());
	stopConnectionFlash ();

	DEBUG_INFO ("STA MAC Address: %s", WiFi.macAddress ().c_str());
	DEBUG_INFO ("AP MAC Address: %s", WiFi.softAPmacAddress().c_str ());
	DEBUG_INFO ("BSSID Address: %s", WiFi.BSSIDstr().c_str ());
	   
	DEBUG_INFO ("IP address: %s", WiFi.localIP ().toString ().c_str ());
	DEBUG_INFO ("WiFi Channel: %d", WiFi.channel ());
	DEBUG_INFO ("WiFi SSID: %s", WiFi.SSID ().c_str ());
	DEBUG_INFO ("Network Name: %s", EnigmaIOTGateway.getNetworkName ());

	initMQTTclient ();

#ifdef ESP32
	//xTaskCreate (EnigmaIOTGateway_handle, "handle", 10000, NULL, 1, &xEnigmaIOTGateway_handle);
	xTaskCreatePinnedToCore (EnigmaIOTGateway_handle, "handle", 4096, NULL, 1, &xEnigmaIOTGateway_handle, 1);
	//xTaskCreatePinnedToCore (client_reconnect, "reconnect", 10000, NULL, 1, &xClient_reconnect, 1);
#endif
	}

void loop () {
	//delay (1);
#ifdef ESP8266
	client.loop ();
#endif

	//String message;
	//if (client.connected ()) {
	//}

#ifdef ESP8266
	EnigmaIOTGateway.handle ();

	if (!client.connected ()) {
		DEBUG_INFO ("reconnect");
		reconnect ();
	}
#endif

	/*while (Serial.available () != 0) {
		message = Serial.readStringUntil ('\n');
		message.trim ();
		if (message[0] == '%') {
			onSerial (message);
		}
	}*/

}
