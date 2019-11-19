/**
  * @file enigmaiot_gateway.ino
  * @version 0.6.0
  * @date 17/11/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  *
  * Communicates with a serial to MQTT gateway to send data to any IoT platform
  */

#include <Arduino.h>
#include <CayenneLPP.h>
#include <FS.h>

#include "dstrootca.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#include "EnigmaIOTGateway.h"
#include "lib/helperFunctions.h"
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
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#include <Update.h>
#include <SPIFFS.h>
#elif defined ESP8266
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <SPI.h>
#endif

#ifndef BUILTIN_LED
#define BUILTIN_LED 5
#endif

#define BLUE_LED BUILTIN_LED
#define RED_LED BUILTIN_LED

#ifdef ESP8266
ETSTimer connectionLedTimer;
#elif defined ESP32
TimerHandle_t connectionLedTimer;
#endif
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
#endif
	char mqtt_user[21];
	char mqtt_pass[41];
} mqttgw_config_t;

mqttgw_config_t mqttgw_config;
const char CONFIG_FILE[] = "/mqtt.conf";
bool shouldSaveConfig = false;

#ifdef SECURE_MQTT
#ifdef ESP8266
BearSSL::X509List certificate (DSTroot_CA);
#endif
WiFiClientSecure secureClient;
PubSubClient client (secureClient);
#else
WiFiClient unsecureClient;
PubSubClient client (unsecureClient);
#endif


AsyncWiFiManagerParameter *mqttServerParam;
AsyncWiFiManagerParameter *mqttPortParam;
AsyncWiFiManagerParameter *mqttUserParam;
AsyncWiFiManagerParameter *mqttPassParam;

String netName;
String clientId;
String gwTopic;


void flashConnectionLed (void* led) {
	//digitalWrite (*(int*)led, !digitalRead (*(int*)led));
	digitalWrite (BUILTIN_LED, !digitalRead (BUILTIN_LED));
}

void startConnectionFlash (int period) {
#ifdef ESP8266
	ets_timer_disarm (&connectionLedTimer);
	if (!connectionLedFlashing) {
		connectionLedFlashing = true;
		ets_timer_arm_new (&connectionLedTimer, period, true, true);
	}
#elif defined ESP32
	if (!connectionLedFlashing) {
		connectionLedFlashing = true;
		connectionLedTimer = xTimerCreate ("led_flash", pdMS_TO_TICKS (period), pdTRUE, (void*)0, flashConnectionLed);
		xTimerStart (connectionLedTimer, 0);
	}
#endif
}

void stopConnectionFlash () {
#ifdef ESP8266
	if (connectionLedFlashing) {
		connectionLedFlashing = false;
		ets_timer_disarm (&connectionLedTimer);
		digitalWrite (connectionLed, HIGH);
	}
#elif defined ESP32
	if (connectionLedFlashing) {
		connectionLedFlashing = false;
		xTimerStop (connectionLedTimer, 0);
		xTimerDelete (connectionLedTimer, 0);
	}
#endif
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

void onDlData (const char* topic, byte* payload, unsigned int length) {}

void processRxControlData (char* macStr, const uint8_t* data, uint8_t length) {
	switch (data[0]) {
		case control_message_type::VERSION_ANS:
			Serial.printf ("~/%s/%s;{\"version\":\"", macStr, GET_VERSION_ANS);
			Serial.write (data + 1, length - 1);
			Serial.println ("\"}");
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
			Serial.printf ("~/%s/%s;{\"rssi\":%d,\"channel\":%u}\n", macStr, GET_RSSI_ANS, (int8_t)data[1], data[2]);
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

	//Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));

	cayennelpp->decode ((uint8_t *)buffer, length, root);
	cayennelpp->CayenneLPP::~CayenneLPP();
	free (cayennelpp);
	
	//Serial.println ();
	Serial.printf ("~/%s/data;", mac_str);
	serializeJson (root, Serial);
	if (lostMessages > 0) {
		//serial.printf ("%u lost messages\n", lostmessages);
		Serial.printf ("~/%s/debug/lostmessages;%u\n", mac_str, lostMessages);
	}
	Serial.printf ("~/%s/status;{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%.2f}\n",
		mac_str,
		EnigmaIOTGateway.getPER ((uint8_t*)mac),
		EnigmaIOTGateway.getErrorPackets ((uint8_t*)mac),
		EnigmaIOTGateway.getTotalPackets ((uint8_t*)mac),
		EnigmaIOTGateway.getPacketsHour ((uint8_t*)mac));
	//jsonBuffer.~BasicJsonDocument();
	//Serial.println ();
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
	Serial.printf ("~/%s/hello\n", macstr);
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
			client.publish (gwTopic.c_str (), "1", true);
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

#ifdef ESP32
void EnigmaIOTGateway_handle (void * param) {
	for (;;) {
		EnigmaIOTGateway.handle ();
		//client.loop ();
		vTaskDelay (1);
	}
}

TaskHandle_t xEnigmaIOTGateway_handle = NULL;

void client_reconnect (void* param) {
	for (;;) {
		if (!client.connected ()) {
			DEBUG_INFO ("reconnect");
			reconnect ();
			vTaskDelay (1);
		}
	}
}

TaskHandle_t xClient_reconnect = NULL;
#endif

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

#ifdef SECURE_MQTT
#ifdef ESP8266
	randomSeed (micros ());
	secureClient.setTrustAnchors (&certificate);
#endif
	secureClient.setCACert (DSTroot_CA);
#endif
	client.setServer (mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
	DEBUG_INFO ("Set MQTT server %s - port %d", mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);

	netName = String (EnigmaIOTGateway.getNetworkName ());
#ifdef ESP8266
	clientId = netName + String (ESP.getChipId (), HEX);
#elif defined ESP32
	uint64_t chipid = ESP.getEfuseMac ();
	clientId = netName + String ((uint32_t)chipid, HEX);
#endif
	gwTopic = netName + String ("/gateway/status");

#ifdef ESP32
	//xTaskCreate (EnigmaIOTGateway_handle, "handle", 10000, NULL, 1, &xEnigmaIOTGateway_handle);
	xTaskCreatePinnedToCore (EnigmaIOTGateway_handle, "handle", 4096, NULL, 2, &xEnigmaIOTGateway_handle, 1);
	xTaskCreatePinnedToCore (client_reconnect, "reconnect", 10000, NULL, 1, &xClient_reconnect, 1);
#endif
	}

void loop () {
	delay (0);
	//String message;

#ifdef ESP8266
	EnigmaIOTGateway.handle ();

	if (!client.connected ()) {
		Serial.println ("reconnect");
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
