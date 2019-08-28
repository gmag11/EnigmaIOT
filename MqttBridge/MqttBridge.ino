/**
  * @file MqttBridge.ino
  * @version 0.3.0
  * @date 28/08/2019
  * @author German Martin
  * @brief Bridge for EnigmaIoT system to forward data from serial to MQTT broker
  *
  * Due to ESP-NOW limitations, it cannot be used with regular WiFi without problems. That's why this bridge is needed.
  * It gets encoded data from serial port and forwards to a MQTT broker.
  *
  * Message format received over serial is in the form of lines like: `~\<address>\<subtopic>;<data>`
  */

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include "bridge_config.h"

#define DEFAULT_BASE_TOPIC "enigmaiot"

//#define BRIDGE_DEBUG

#define DEBUG_LINE_PREFIX() DEBUG_ESP_PORT.printf ("[%lu] %lu free (%s:%d) ",millis(),(unsigned long)ESP.getFreeHeap(),__FUNCTION__,__LINE__);

#ifdef BRIDGE_DEBUG
#define DEBUG_ESP_PORT Serial
#define _DEBUG_(...) DEBUG_LINE_PREFIX(); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
#else
#define _DEBUG_(...)
#endif

typedef struct {
	char mqtt_server[41];
	int mqtt_port = 8883;
	char mqtt_user[21];
	char mqtt_pass[41];
	char base_topic[21] = DEFAULT_BASE_TOPIC;
} bridge_config_t;

ETSTimer ledTimer;
const int notifLed = BUILTIN_LED;
boolean ledFlashing = false;

bridge_config_t bridgeConfig;
const char CONFIG_FILE[] = "/config.txt";
bool shouldSaveConfig = false;

#ifdef SECURE_MQTT
BearSSL::X509List cert (DSTrootCA);
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
PubSubClient client (espClient);

void onDlData (const char* topic, byte* payload, unsigned int length) {
	String topicStr (topic);

	topicStr.replace (String (bridgeConfig.base_topic), "");

	Serial.printf ("%%%s;", topicStr.c_str ());
	for (unsigned int i = 0; i < length; i++) {
		Serial.print ((char)(payload[i]));
	}
	Serial.println ();
}

#ifdef SECURE_MQTT
void setClock () {
	configTime (2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
#ifdef BRIDGE_DEBUG
	Serial.print ("\nWaiting for NTP time sync: ");
	time_t now = time (nullptr);
	while (now < 8 * 3600 * 2) {
		delay (500);
		Serial.print (".");
		now = time (nullptr);
	}
	//Serial.println ("");
	struct tm timeinfo;
	gmtime_r (&now, &timeinfo);
	Serial.print ("Current time: ");
	Serial.print (asctime (&timeinfo));
#endif
}
#endif

void reconnect () {
	// Loop until we're reconnected
	while (!client.connected ()) {
		startFlash(500);
		Serial.print ("Attempting MQTT connection...");
		// Create a random client ID
		String clientId = bridgeConfig.base_topic + String (ESP.getChipId (), HEX);
		String gwTopic = bridgeConfig.base_topic + String ("/gateway/status");
		// Attempt to connect
#ifdef SECURE_MQTT
		setClock ();
#endif
		if (client.connect (clientId.c_str (), bridgeConfig.mqtt_user, bridgeConfig.mqtt_pass, gwTopic.c_str (), 0, 1, "0", true)) {
			Serial.println ("connected");
			// Once connected, publish an announcement...
			//String gwTopic = BASE_TOPIC + String("/gateway/hello");
			client.publish (gwTopic.c_str (), "1");
			// ... and resubscribe
			String dlTopic = bridgeConfig.base_topic + String ("/+/set/#");
			client.subscribe (dlTopic.c_str ());
			dlTopic = bridgeConfig.base_topic + String ("/+/get/#");
			client.subscribe (dlTopic.c_str ());
			client.setCallback (onDlData);
			stopFlash ();
		}
		else {
			Serial.print ("failed, rc=");
			Serial.print (client.state ());
			Serial.println (" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay (5000);
		}
	}
}

void flashLed (void *led) {
	digitalWrite (*(int*)led, !digitalRead (*(int*)led));
}

void startFlash (int period) {
	ets_timer_disarm (&ledTimer);
	if (!ledFlashing) {
		ledFlashing = true;
		ets_timer_arm_new (&ledTimer, period, true, true);
	}
}

void stopFlash () {
	if (ledFlashing) {
		ledFlashing = false;
		ets_timer_disarm (&ledTimer);
		digitalWrite (notifLed, HIGH);
	}
}

void saveConfigCallback () {
	shouldSaveConfig = true;
    _DEBUG_ ("Should save configuration");

}

bool configWiFiManager () {
	AsyncWebServer server (80);
	DNSServer dns;

	char port[10];
	itoa (bridgeConfig.mqtt_port, port, 10);
	//String (bridgeConfig.mqtt_port).toCharArray (port, 6);

	AsyncWiFiManager wifiManager (&server, &dns);
	AsyncWiFiManagerParameter mqttServerParam ("mqttserver", "MQTT Server", bridgeConfig.mqtt_server, 41, "required type=\"text\" maxlength=40");
	AsyncWiFiManagerParameter mqttPortParam ("mqttport", "MQTT Port", port, 6, "required type=\"number\" min=\"0\" max=\"65535\" step=\"1\"");
	AsyncWiFiManagerParameter mqttUserParam ("mqttuser", "MQTT User", bridgeConfig.mqtt_user, 21, "required type=\"text\" maxlength=20");
	AsyncWiFiManagerParameter mqttPassParam ("mqttpass", "MQTT Password", bridgeConfig.mqtt_pass, 41, "required type=\"text\" maxlength=40");
	AsyncWiFiManagerParameter mqttBaseTopicParam ("mqtttopic", "MQTT Base Topic", bridgeConfig.base_topic, 21, "required type=\"text\" maxlength=20");

	wifiManager.addParameter (&mqttServerParam);
	wifiManager.addParameter (&mqttPortParam);
	wifiManager.addParameter (&mqttUserParam);
	wifiManager.addParameter (&mqttPassParam);
	wifiManager.addParameter (&mqttBaseTopicParam);

	wifiManager.setSaveConfigCallback (saveConfigCallback);
	wifiManager.setTryConnectDuringConfigPortal (true);
	wifiManager.setConnectTimeout (30);
	wifiManager.setConfigPortalTimeout (60);
#ifndef BRIDGE_DEBUG
	wifiManager.setDebugOutput (false);
#endif
    _DEBUG_ ("Connecting to WiFi: %s", WiFi.SSID ().c_str());
	String apname = "EnigmaIoTMQTTBridge" + String (ESP.getChipId (), 16);
	boolean result = wifiManager.autoConnect (apname.c_str());

	if (shouldSaveConfig) {
        _DEBUG_ ("==== Config Portal result ====");
        _DEBUG_ ("SSID: %s", WiFi.SSID().c_str());
		memcpy (bridgeConfig.mqtt_server, mqttServerParam.getValue (), mqttServerParam.getValueLength ());
		bridgeConfig.mqtt_port = atoi (mqttPortParam.getValue ());
		memcpy (bridgeConfig.mqtt_user, mqttUserParam.getValue (), mqttUserParam.getValueLength ());
		memcpy (bridgeConfig.mqtt_pass, mqttPassParam.getValue (), mqttPassParam.getValueLength ());
		memcpy (bridgeConfig.base_topic, mqttBaseTopicParam.getValue (), mqttBaseTopicParam.getValueLength ());
	}
	return result;
}

bool loadBridgeConfig () {
	//SPIFFS.remove (CONFIG_FILE); // Only for testing

	if (SPIFFS.exists (CONFIG_FILE)) {
        _DEBUG_ ("Opening %s file", CONFIG_FILE);
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
            _DEBUG_ ("%s opened", CONFIG_FILE);
			size_t size = configFile.size ();
			if (size < sizeof (bridge_config_t)) {
                _DEBUG_ ("Config file is corrupted. Deleting");
				SPIFFS.remove (CONFIG_FILE);
				return false;
			}
			configFile.read ((uint8_t*)(&bridgeConfig), sizeof (bridge_config_t));
			configFile.close ();
            _DEBUG_ ("Gateway configuration successfuly read");
			return true;
		}
	}
	else {
        _DEBUG_ ("%s do not exist", CONFIG_FILE);
		return false;
	}

	return false;
}

bool saveBridgeConfig () {
	File configFile = SPIFFS.open (CONFIG_FILE, "w");
	if (!configFile) {
        _DEBUG_ ("Failed to open config file %s for writing", CONFIG_FILE);
		return false;
	}
	configFile.write ((uint8_t*)(&bridgeConfig), sizeof (bridge_config_t));
	configFile.close ();
    _DEBUG_ ("Gateway configuration saved to flash");
	return true;
}

void setup ()
{
	Serial.begin (115200);
	ets_timer_setfn (&ledTimer, flashLed, (void*)&notifLed);
	pinMode (BUILTIN_LED, OUTPUT);
	digitalWrite (BUILTIN_LED, HIGH);
	startFlash (500);
	if (!SPIFFS.begin ()) {
        _DEBUG_ ("Error starting filesystem. Formatting.");
		SPIFFS.format ();
		delay (5000);
		ESP.restart ();
	}
	if (!loadBridgeConfig ()) {
        _DEBUG_ ("Cannot load configuration");
	}
	if (!configWiFiManager ()) {
        _DEBUG_ ("Error connecting to WiFi");
		delay (2000);
		ESP.restart ();
	}
	if (shouldSaveConfig) {
		if (!saveBridgeConfig ()) {
            _DEBUG_ ("Error writting filesystem. Formatting.");
			SPIFFS.format ();
			delay (5000);

			ESP.restart ();
		}
	}
	
#ifdef SECURE_MQTT
	randomSeed (micros ());
	espClient.setTrustAnchors (&cert);
#endif
	client.setServer (bridgeConfig.mqtt_server, bridgeConfig.mqtt_port);

}

void loop ()
{
	String message;
	String topic;
	String data;
	bool dataPresent = false;

	if (!client.connected ()) {
		Serial.println ("reconnect");
		reconnect ();
	}

	client.loop ();

	while (Serial.available () != 0) {
		message = Serial.readStringUntil ('\n');
        _DEBUG_ ("Message: %s", message.c_str ());
		dataPresent = true;
		if (message.length () > 0) {
			break;
		}
	}

	if (dataPresent) {
		dataPresent = false;
		if (message[0] == '~') {
            _DEBUG_ ("New message");
			int end = message.indexOf (';');
			topic = bridgeConfig.base_topic + message.substring (1, end);
            _DEBUG_ ("Topic: %s", topic.c_str ());
			if (end < ((int)(message.length ()) - 1) && end > 0) {
				data = message.substring (end + 1);
                _DEBUG_ (" With data");
                _DEBUG_ (" -- Data: %s", data.c_str ());
			}
			else {
				data = "";
                _DEBUG_ (" Without data\n");
			}
            _DEBUG_ ("Publish %s : %s", topic.c_str (), data.c_str ());
			if (client.beginPublish (topic.c_str (), data.length (), false)) {
				client.write ((const uint8_t*)data.c_str (), data.length ());
				if (client.endPublish () == 1) {
                    _DEBUG_ ("Publish OK");
				}
				else {
                    _DEBUG_ ("Publish error");
				}
			}
			else {
                _DEBUG_ ("Publish error");
			}
		}
	}

}
