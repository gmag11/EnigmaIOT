/**
  * @file MqttBridge.ino
  * @version 0.2.0
  * @date 28/06/2019
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

#define BRIDGE_DEBUG

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
#ifdef BRIDGE_DEBUG
	Serial.println ("Should save configuration");
#endif

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
	wifiManager.setConnectTimeout (30);
	wifiManager.setConfigPortalTimeout (300);
#ifndef BRIDGE_DEBUG
	wifiManager.setDebugOutput (false);
#endif
	String apname = "EnigmaIoTMQTTBridge" + String (ESP.getChipId (), 16);
	boolean result = wifiManager.autoConnect (apname.c_str());

	if (shouldSaveConfig) {
#ifdef BRIDGE_DEBUG
		Serial.println ("==== Config Portal result ====");
		Serial.printf ("SSID: %s\n", WiFi.SSID().c_str());
#endif
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
#ifdef BRIDGE_DEBUG
		Serial.printf ("Opening %s file\n", CONFIG_FILE);
#endif
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
#ifdef BRIDGE_DEBUG
			Serial.printf ("%s opened\n", CONFIG_FILE);
#endif
			size_t size = configFile.size ();
			if (size < sizeof (bridge_config_t)) {
#ifdef BRIDGE_DEBUG
				Serial.println ("Config file is corrupted. Deleting");
#endif
				SPIFFS.remove (CONFIG_FILE);
				return false;
			}
			configFile.read ((uint8_t*)(&bridgeConfig), sizeof (bridge_config_t));
			configFile.close ();
#ifdef BRIDGE_DEBUG
			Serial.println ("Gateway configuration successfuly read");
#endif
			return true;
		}
	}
	else {
#ifdef BRIDGE_DEBUG
		Serial.printf ("%s do not exist\n", CONFIG_FILE);
#endif
		return false;
	}

	return false;
}

bool saveBridgeConfig () {
	File configFile = SPIFFS.open (CONFIG_FILE, "w");
	if (!configFile) {
#ifdef BRIDGE_DEBUG
		Serial.printf ("Failed to open config file %s for writing\n", CONFIG_FILE);
#endif
		return false;
	}
	configFile.write ((uint8_t*)(&bridgeConfig), sizeof (bridge_config_t));
	configFile.close ();
#ifdef BRIDGE_DEBUG
	Serial.println ("Gateway configuration saved to flash");
#endif 
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
#ifdef BRIDGE_DEBUG
		Serial.println ("Error starting filesystem. Formatting.");
#endif
		SPIFFS.format ();
		delay (5000);
		ESP.reset ();
	}
	if (!loadBridgeConfig ()) {
#ifdef BRIDGE_DEBUG
		Serial.println ("Cannot load configuration");
#endif
	}
	if (!configWiFiManager ()) {
#ifdef BRIDGE_DEBUG
		Serial.println ("Error connecting to WiFi");
#endif
		delay (2000);
		ESP.reset ();
	}
	if (shouldSaveConfig) {
		if (!saveBridgeConfig ()) {
#ifdef BRIDGE_DEBUG
			Serial.println ("Error writting filesystem. Formatting.");
#endif
			SPIFFS.format ();
			delay (5000);

			ESP.reset ();
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
#ifdef BRIDGE_DEBUG
		Serial.printf ("Message: %s\n", message.c_str ());
#endif
		dataPresent = true;
		if (message.length () > 0) {
			break;
		}
	}

	if (dataPresent) {
		dataPresent = false;
		if (message[0] == '~') {
#ifdef BRIDGE_DEBUG
			Serial.println ("New message");
#endif
			int end = message.indexOf (';');
			topic = bridgeConfig.base_topic + message.substring (1, end);
#ifdef BRIDGE_DEBUG
			Serial.printf ("Topic: %s", topic.c_str ());
#endif
			if (end < ((int)(message.length ()) - 1) && end > 0) {
				data = message.substring (end + 1);
#ifdef BRIDGE_DEBUG
				Serial.println (" With data");
				Serial.printf (" -- Data: %s\n", data.c_str ());
#endif
			}
			else {
				data = "";
#ifdef BRIDGE_DEBUG
				Serial.println (" Without data");
				Serial.println ();
#endif
			}
#ifdef BRIDGE_DEBUG
			Serial.printf ("Publish %s : %s\n", topic.c_str (), data.c_str ());
#endif
			if (client.beginPublish (topic.c_str (), data.length (), false)) {
				client.write ((const uint8_t*)data.c_str (), data.length ());
				if (client.endPublish () == 1) {
#ifdef BRIDGE_DEBUG
					Serial.println ("Publish OK");
				}
				else {
					Serial.println ("Publish error");
#endif
				}
#ifdef BRIDGE_DEBUG
			}
			else {
				Serial.println ("Publish error");
#endif
			}
		}
	}

}
