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
  * Message format received over serial is in the form of lines like: `~\<address>\<subtopic>;<data>
  */


#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "bridge_config.h"

#ifndef BRIDGE_CONFIG_H
#define SSID "ssid"
#define PASSWD "passwd"
#endif

const char* BASE_TOPIC = "enigmaiot";

WiFiClient espClient;
PubSubClient client (espClient);

void reconnect () {
    // Loop until we're reconnected
    while (!client.connected ()) {
        Serial.print ("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String (random (0xffff), HEX);
        // Attempt to connect
        if (client.connect (clientId.c_str (),MQTT_USER,MQTT_PASS)) {
            Serial.println ("connected");
            // Once connected, publish an announcement...
            String gwTopic = BASE_TOPIC + String("/gateway/hello");
            client.publish (gwTopic.c_str(), "");
            // ... and resubscribe
            client.subscribe ("inTopic");
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

void setup()
{
    Serial.begin (115200);
    WiFi.mode (WIFI_STA);
    WiFi.begin (SSID, PASSWD);
    while (WiFi.status () != WL_CONNECTED) {
        delay (500);
        Serial.print (".");
    }
    randomSeed (micros ());
    client.setServer (mqtt_server, 1883);

}

void loop()
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
        Serial.printf ("Message: %s\n", message.c_str());
        dataPresent = true;
        if (message.length () > 0) {
            break;
        }
    }

    if (dataPresent) {
        dataPresent = false;
        if (message[0] == '~') {
            Serial.println ("New message");
            int end = message.indexOf (';');
            topic = BASE_TOPIC + message.substring (1, end);
            Serial.printf ("Topic: %s", topic.c_str ());
            if (end < message.length () - 1) {
                Serial.println (" With data");
                data = message.substring (end + 1);
                Serial.printf (" -- Data: %s\n", data.c_str ());
            }
            else {
                Serial.println (" Without data");
                data = "";
                Serial.println ();
            }
            Serial.printf ("Publish %s : %s\n",topic.c_str(), data.c_str());
            if (client.beginPublish (topic.c_str (), data.length (), false)) {
                client.write ((const uint8_t *)data.c_str (), data.length ());
                if (client.endPublish() == 1) {
                    Serial.println ("Publish OK");
                } else {
                    Serial.println ("Publish error");
                }
            } else {
                Serial.println ("Publish error");
            }
        }
    }

}
