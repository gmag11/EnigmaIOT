/**
  * @file MqttBridge.ino
  * @version 0.1.0
  * @date 09/03/2019
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


#ifndef BRIDGE_CONFIG_H
#define SSID "ssid"
#define PASSWD "passwd"
#endif

//#define BRIDGE_DEBUG

typedef struct {
	char mqtt_server[41];
	int mqtt_port = 8883;
	char mqtt_user[21];
	char mqtt_pass[41];
	char base_topic[21] = "enigmaiot";
} bridge_config_t;

//const char* BASE_TOPIC = "enigmaiot";
ETSTimer ledTimer;
const int notifLed = BUILTIN_LED;
boolean ledFlashing = false;

bridge_config_t bridgeConfig;

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

	Serial.printf ("*%s;", topicStr.c_str ());
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

	wifiManager.setConnectTimeout (60);
	String apname = "EnigmaIoTMQTTBridge" + String (ESP.getChipId (), 16);
	boolean result = wifiManager.autoConnect (apname.c_str());

	if (result) {
		//DEBUG_DBG ("==== Config Portal result ====");
		//DEBUG_DBG ("SSID: %s", wifiManager.);
		memcpy (bridgeConfig.mqtt_server, mqttServerParam.getValue (), mqttServerParam.getValueLength ());
		bridgeConfig.mqtt_port = atoi (mqttPortParam.getValue ());
		memcpy (bridgeConfig.mqtt_user, mqttUserParam.getValue (), mqttUserParam.getValueLength ());
		memcpy (bridgeConfig.mqtt_pass, mqttPassParam.getValue (), mqttPassParam.getValueLength ());
		memcpy (bridgeConfig.base_topic, mqttBaseTopicParam.getValue (), mqttBaseTopicParam.getValueLength ());
	}
	return result;
}

bool loadBridgeConfig () {
	return false;
}

bool saveBridgeConfig () {
	return false;
}

void setup ()
{
	Serial.begin (115200);
	ets_timer_setfn (&ledTimer, flashLed, (void*)&notifLed);
	pinMode (BUILTIN_LED, OUTPUT);
	digitalWrite (BUILTIN_LED, HIGH);
	startFlash (500);
	if (!loadBridgeConfig ()) {
		if (configWiFiManager ()) {
			if (!saveBridgeConfig ()) {
			}
		}
	}
	//WiFi.mode (WIFI_STA);
	//WiFi.begin (SSID, PASSWD);
	//while (WiFi.status () != WL_CONNECTED) {
	//	delay (500);
	//	Serial.print (".");
	//}
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
