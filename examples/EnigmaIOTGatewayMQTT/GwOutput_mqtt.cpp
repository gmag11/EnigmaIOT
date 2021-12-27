/**
  * @file GwOutput_mqtt.cpp
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief MQTT Gateway output module
  *
  * Module to send and receive EnigmaIOT information from MQTT broker
  */

#include <Arduino.h>
#include "GwOutput_mqtt.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <helperFunctions.h>
#include <EnigmaIOTdebug.h>
#include <PubSubClient.h>

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <SPI.h>
#ifdef SECURE_MQTT
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif // SECURE_MQTT
#endif // ESP32

#include <FS.h>


GwOutput_MQTT GwOutput;

void GwOutput_MQTT::configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw) {
	enigmaIotGateway = enigmaIotGw;
	mqttServerParam = new AsyncWiFiManagerParameter ("mqttserver", "MQTT Server", mqttgw_config.mqtt_server, 41, "required type=\"text\" maxlength=40");
	char port[10];
	itoa (mqttgw_config.mqtt_port, port, 10);
	mqttPortParam = new AsyncWiFiManagerParameter ("mqttport", "MQTT Port", port, 6, "required type=\"number\" min=\"0\" max=\"65535\" step=\"1\"");
	mqttUserParam = new AsyncWiFiManagerParameter ("mqttuser", "MQTT User", mqttgw_config.mqtt_user, 21, "required type=\"text\" maxlength=20");
	mqttPassParam = new AsyncWiFiManagerParameter ("mqttpass", "MQTT Password", "", 41, "type=\"password\" maxlength=40");

	enigmaIotGateway->addWiFiManagerParameter (mqttServerParam);
	enigmaIotGateway->addWiFiManagerParameter (mqttPortParam);
	enigmaIotGateway->addWiFiManagerParameter (mqttUserParam);
	enigmaIotGateway->addWiFiManagerParameter (mqttPassParam);

}

bool GwOutput_MQTT::saveConfig () {
    if (!FILESYSTEM.begin ()) {
		DEBUG_WARN ("Error opening filesystem");
	}
	DEBUG_DBG ("Filesystem opened");

    File configFile = FILESYSTEM.open (CONFIG_FILE, "w");
	if (!configFile) {
		DEBUG_WARN ("Failed to open config file %s for writing", CONFIG_FILE);
		return false;
	} else {
		DEBUG_DBG ("%s opened for writting", CONFIG_FILE);
	}

	const size_t capacity = JSON_OBJECT_SIZE (4) + 110;
	DynamicJsonDocument doc (capacity);

	doc["mqtt_server"] = mqttgw_config.mqtt_server;
	doc["mqtt_port"] = mqttgw_config.mqtt_port;
	doc["mqtt_user"] = mqttgw_config.mqtt_user;
	doc["mqtt_pass"] = mqttgw_config.mqtt_pass;

	if (serializeJson (doc, configFile) == 0) {
		DEBUG_ERROR ("Failed to write to file");
		configFile.close ();
        //FILESYSTEM.remove (CONFIG_FILE); // Testing only
		return false;
	}

	String output;
	serializeJsonPretty (doc, output);

	DEBUG_DBG ("%s", output.c_str ());

	configFile.flush ();
	//size_t size = configFile.size ();

	configFile.close ();
	DEBUG_DBG ("Gateway configuration saved to flash. %u bytes", configFile.size ());
	return true;
}

bool GwOutput_MQTT::loadConfig () {
    //FILESYSTEM.remove (CONFIG_FILE); // Only for testing
	bool json_correct = false;

    if (!FILESYSTEM.begin ()) {
		DEBUG_WARN ("Error starting filesystem. Formatting");
        FILESYSTEM.format ();
		WiFi.disconnect ();
	}

    if (FILESYSTEM.exists (CONFIG_FILE)) {

		DEBUG_DBG ("Opening %s file", CONFIG_FILE);
        File configFile = FILESYSTEM.open (CONFIG_FILE, "r");
		if (configFile) {
			//size_t size = configFile.size ();
            DEBUG_DBG ("%s opened. %u bytes", CONFIG_FILE, configFile.size ());

			const size_t capacity = JSON_OBJECT_SIZE (4) + 110;
			DynamicJsonDocument doc (capacity);

			DeserializationError error = deserializeJson (doc, configFile);

			if (error) {
				DEBUG_ERROR ("Failed to parse file");
			} else {
				DEBUG_DBG ("JSON file parsed");
			}

			if (doc.containsKey ("mqtt_server") && doc.containsKey ("mqtt_port")
				&& doc.containsKey ("mqtt_user") && doc.containsKey ("mqtt_pass")) {
				json_correct = true;
			}

			strncpy (mqttgw_config.mqtt_server, doc["mqtt_server"] | "", sizeof (mqttgw_config.mqtt_server));
			mqttgw_config.mqtt_port = doc["mqtt_port"].as<int> ();
			strncpy (mqttgw_config.mqtt_user, doc["mqtt_user"] | "", sizeof (mqttgw_config.mqtt_user));
			strncpy (mqttgw_config.mqtt_pass, doc["mqtt_pass"] | "", sizeof (mqttgw_config.mqtt_pass));

			configFile.close ();
			if (json_correct) {
				DEBUG_INFO ("MQTT output module configuration successfuly read");
			}
			DEBUG_DBG ("==== MQTT Configuration ====");
			DEBUG_DBG ("MQTT server: %s", mqttgw_config.mqtt_server);
			DEBUG_DBG ("MQTT port: %d", mqttgw_config.mqtt_port);
			DEBUG_DBG ("MQTT user: %s", mqttgw_config.mqtt_user);
			DEBUG_VERBOSE ("MQTT password: %s", mqttgw_config.mqtt_pass);

			String output;
			serializeJsonPretty (doc, output);

			DEBUG_DBG ("JSON file %s", output.c_str ());

		} else {
			DEBUG_WARN ("Error opening %s", CONFIG_FILE);
		}
	} else {
		DEBUG_WARN ("%s do not exist", CONFIG_FILE);
	}

	return json_correct;
}


void GwOutput_MQTT::configManagerExit (bool status) {
	DEBUG_INFO ("==== Config Portal MQTTGW result ====");
	DEBUG_INFO ("MQTT server: %s", mqttServerParam->getValue ());
	DEBUG_INFO ("MQTT port: %s", mqttPortParam->getValue ());
	DEBUG_INFO ("MQTT user: %s", mqttUserParam->getValue ());
	DEBUG_INFO ("MQTT password: %s", mqttPassParam->getValue ());
	DEBUG_INFO ("Status: %s", status ? "true" : "false");

	if (status && EnigmaIOTGateway.getShouldSave ()) {
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
		if (!saveConfig ()) {
			DEBUG_ERROR ("Error writting MQTT config to filesystem.");
		} else {
			DEBUG_INFO ("Configuration stored");
		}
	} else {
		DEBUG_DBG ("Configuration does not need to be saved");
	}

	delete (mqttServerParam);
	delete (mqttPortParam);
	delete (mqttUserParam);
	delete (mqttPassParam);
}

bool GwOutput_MQTT::begin () {
    //this->mqtt_queue = new EnigmaIOTRingBuffer<mqtt_queue_item_t> (MAX_MQTT_QUEUE_SIZE);
#ifdef SECURE_MQTT
	randomSeed (micros ());
#ifdef ESP32
	espClient.setCACert (DSTroot_CA);
#elif defined(ESP8266)
	espClient.setTrustAnchors (&certificate);
#endif // ESP32
	DEBUG_INFO ("CA store set");
#endif // SECURE_MQTT
	mqtt_client.setServer (mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
	DEBUG_INFO ("Set MQTT server %s - port %d", mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
	mqtt_client.setBufferSize (MQTT_BUFFER_SIZE);
	netName = String (EnigmaIOTGateway.getNetworkName ());

#ifdef ESP32
	uint64_t chipid = ESP.getEfuseMac ();
	clientId = netName + String ((uint32_t)chipid, HEX);
#elif defined(ESP8266)
	clientId = netName + String (ESP.getChipId (), HEX);
#endif // ESP32
    
    configTime (0, 0, NTP_SERVER_1, NTP_SERVER_2);
    setenv ("TZ", TZINFO, 1);
    tzset ();

	gwTopic = netName + GW_STATUS;
	reconnect ();
	return true;
}

void GwOutput_MQTT::reconnect () {
	// Loop until we're reconnected
	while (!mqtt_client.connected ()) {
		// TODO: startConnectionFlash (500);
		DEBUG_INFO ("Attempting MQTT connection...");
		for (int i = 1; i < 5; i++) {
			DEBUG_WARN ("Connecting to WiFi %s", WiFi.SSID ().c_str ());
			delay (1000);
			if (WiFi.isConnected ()) {
				DEBUG_WARN ("WiFi is connected");
				break;
			} else {
				WiFi.begin ();
			}
		}
		// Create a random client ID
		// Attempt to connect
//#ifdef SECURE_MQTT
		setClock ();
//#endif
		DEBUG_DBG ("Clock set.");
		DEBUG_DBG ("Connect to MQTT server: user %s, pass %s, topic %s",
				   mqttgw_config.mqtt_user, mqttgw_config.mqtt_pass, gwTopic.c_str ());
		//client.setServer (mqttgw_config.mqtt_server, mqttgw_config.mqtt_port);
		if (mqtt_client.connect (clientId.c_str (), mqttgw_config.mqtt_user, mqttgw_config.mqtt_pass, gwTopic.c_str (), 0, true, "0", true)) {
			DEBUG_WARN ("MQTT connected");
			// Once connected, publish an announcement...
			publishMQTT (gwTopic.c_str (), "1", 1, true);
			// ... and resubscribe
			String dlTopic = netName + String ("/+/set/#");
			mqtt_client.subscribe (dlTopic.c_str ());
			dlTopic = netName + String ("/+/get/#");
			mqtt_client.subscribe (dlTopic.c_str ());
			mqtt_client.setCallback (onDlData);
			// TODO: stopConnectionFlash ();
		} else {
			mqtt_client.disconnect ();
			DEBUG_ERROR ("failed, rc=%d try again in 5 seconds", mqtt_client.state ());
#ifdef SECURE_MQTT
			char error[100];
#ifdef ESP8266
			int errorCode = espClient.getLastSSLError (error, 100);
#elif defined ESP32
			int errorCode = espClient.lastError (error, 100);
#endif
			DEBUG_ERROR ("Connect error %d: %s", errorCode, error);
#endif
			// Wait 5 seconds before retrying
#ifdef ESP32
			const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;
			vTaskDelay (xDelay);
#else
			delay (5000);
#endif
		}
	}
}

char* getTopicAddress (char* topic, unsigned int& len) {
	if (!topic)
        return nullptr;

	char* start = strchr (topic, '/') + 1;
	char* end;

	if (start) {
		end = strchr (start, '/');
	} else {
        return nullptr;
	}
	//DEBUG_INFO ("Start %p : %d", start, start - topic);
	//DEBUG_INFO ("End %p : %d", end, end - topic);
	if (end) {
		len = end - start;
	} else {
		len = strlen (topic) - (start - topic);
	}

	return start;
}

control_message_type_t checkMsgType (String data) {
	DEBUG_DBG ("Type topic: %s", data.c_str ());
	if (data == GET_VERSION) {
		return control_message_type::VERSION;
	} else if (data == GET_SLEEP) {
		return control_message_type::SLEEP_GET;
	} else if (data == SET_SLEEP) {
		return control_message_type::SLEEP_SET;
	} else if (data == SET_OTA) {
		return control_message_type::OTA;
	} else if (data == SET_IDENTIFY) {
		DEBUG_WARN ("IDENTIFY MESSAGE %s", data.c_str ());
		return control_message_type::IDENTIFY;
	} else if (data == SET_RESET_CONFIG) {
		DEBUG_WARN ("RESET CONFIG MESSAGE %s", data.c_str ());
		return control_message_type::RESET;
	} else if (data == GET_RSSI) {
		DEBUG_INFO ("GET RSSI MESSAGE %s", data.c_str ());
		return control_message_type::RSSI_GET;
	} else if (data == SET_USER_DATA) {
		DEBUG_INFO ("USER DATA %s", data.c_str ());
		return control_message_type::USERDATA_SET;
	} else if (data == GET_USER_DATA) {
		DEBUG_INFO ("USER DATA %s", data.c_str ());
		return control_message_type::USERDATA_GET;
	} else if (data == GET_NAME) {
		DEBUG_INFO ("GET NODE NAME AND ADDRESS");
		return control_message_type::NAME_GET;
	} else if (data == SET_NAME) {
		DEBUG_INFO ("SET NODE NAME %s", data.c_str ());
		return control_message_type::NAME_SET;
	} else if (data == SET_RESTART_MCU) {
		DEBUG_INFO ("RESET MCU");
		return control_message_type::RESTART_NODE;
	} else
		return control_message_type::INVALID;
}

control_message_type_t getTopicType (char* topic, char*& userCommand) {
	if (!topic)
		return control_message_type::INVALID;

	String command;

	//Discard address
	char* start = strchr (topic, '/') + 1;
	if (start)
		start = strchr (start, '/') + 1;
	else
		return control_message_type::INVALID;
	//DEBUG_INFO ("Second Start %p", start);
	if ((int)start > 0x01) { // TODO: Why this condition ????
		command = String (start);
		userCommand = start;
	} else {
		return control_message_type::INVALID;
	}
	//DEBUG_INFO ("Start %p : %d", start, start - topic);
	//DEBUG_INFO ("Command %s", command.c_str());

	control_message_type_t msgType = checkMsgType (command);

	return msgType;
}


void GwOutput_MQTT::onDlData (char* topic, uint8_t* data, unsigned int len) {
	uint8_t addr[ENIGMAIOT_ADDR_LEN];
	char* addressStr;
	control_message_type_t msgType;
	char* userCommand;
    char* nodeName = nullptr;


	DEBUG_DBG ("Topic %s", topic);

	unsigned int addressLen;

	addressStr = getTopicAddress (topic, addressLen);

	if (addressStr) {
		//DEBUG_INFO ("Len: %u", addressLen);
		DEBUG_DBG ("Address %.*s", addressLen, addressStr);
		if (!str2mac (addressStr, addr)) {
			DEBUG_INFO ("Not a mac address. Treating it as a node name");
			if (addressLen) {
				nodeName = (char*)calloc (addressLen + 1, sizeof (char));
				memcpy (nodeName, addressStr, addressLen);
			} else {
				DEBUG_WARN ("Invalid address");
				return;
			}
		} else {
			DEBUG_DBG ("Hex Address = %s", printHexBuffer (addr, 6));
		}
	} else
		return;

	msgType = getTopicType (topic, userCommand);


	DEBUG_DBG ("User command: %s", userCommand);
	DEBUG_DBG ("MsgType 0x%02X", msgType);
	DEBUG_DBG ("Data: %.*s\n", len, data);

	if (msgType != control_message_type_t::INVALID) {
		GwOutput.downlinkCb (addr, nodeName, msgType, (char*)data, len);
	} else {
		DEBUG_WARN ("Invalid message");
	}

	if (nodeName) {
		free (nodeName);
		nodeName = NULL;
	}
}

void GwOutput_MQTT::loop () {

	mqtt_client.loop ();
	if (!mqtt_client.connected ()) {
		reconnect ();
	} else {
        mqtt_queue_item_t* message;
        static time_t statusLastUpdated;

		if (!mqtt_queue.empty ()) {
			message = getMQTTqueue ();
			if (publishMQTT (message->topic, message->payload, message->payload_len, message->retain)) {
				DEBUG_DBG ("MQTT published. %s %.*s", message->topic, message->payload_len, message->payload);
				popMQTTqueue ();
			}
		}
		if (millis () - statusLastUpdated > STATUS_SEND_PERIOD) {
			statusLastUpdated = millis ();
			publishMQTT (gwTopic.c_str (), "1", 1, true);
		}
	}
}

bool GwOutput_MQTT::publishMQTT (const char* topic, const char* payload, size_t len, bool retain) {
	DEBUG_INFO ("Publish MQTT. %s : %.*s", topic, len, payload);
	if (mqtt_client.connected ()) {
		return mqtt_client.publish (topic, (uint8_t*)payload, len, retain);
	} else {
		DEBUG_WARN ("MQTT client not connected");
		return false;
	}
}

//#ifdef SECURE_MQTT
void GwOutput_MQTT::setClock () {
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
//#endif

bool GwOutput_MQTT::addMQTTqueue (const char* topic, char* payload, size_t len, bool retain) {
    mqtt_queue_item_t message;

	if (mqtt_queue.size () >= MAX_MQTT_QUEUE_SIZE) {
		mqtt_queue.pop ();
	}

	//message.topic = (char*)malloc (strlen (topic) + 1);
	strncpy (message.topic, topic, MAX_MQTT_TOPIC_LEN);
    message.payload_len = len < MAX_MQTT_PLD_LEN ? len : MAX_MQTT_PLD_LEN;
	//message->payload = (char*)malloc (len);
    memcpy (message.payload, payload, message.payload_len);
	message.retain = retain;

	mqtt_queue.push (&message);
	DEBUG_DBG ("%d MQTT messages queued Len:%d %s %.*s", mqtt_queue.size (),
			   len,
			   message.topic,
			   message.payload_len, message.payload);

	return true;
}

mqtt_queue_item_t* GwOutput_MQTT::getMQTTqueue () {
	if (mqtt_queue.size ()) {
		DEBUG_DBG ("MQTT message got from queue");
		return mqtt_queue.front ();
	}
    return nullptr;
}

void GwOutput_MQTT::popMQTTqueue () {
	if (mqtt_queue.size ()) {
        mqtt_queue_item_t* message;

		message = mqtt_queue.front ();
		if (message) {
			if (message->topic) {
				//delete(message->topic);
                message->topic[0] = 0;
			}
			if (message->payload) {
				//delete(message->payload);
                message->payload[0] = 0;
			}
            message->payload_len = 0;
			//delete message;
		}
		mqtt_queue.pop ();
		DEBUG_DBG ("MQTT message pop. Size %d", mqtt_queue.size ());
	}
}

#if SUPPORT_HA_DISCOVERY
bool GwOutput_MQTT::rawMsgSend (const char* topic, char* payload, size_t len, bool retain) {
    bool result;
    
    if ((result = addMQTTqueue (topic, payload, len, retain))) {
        DEBUG_INFO ("MQTT queued %s. Length %d", topic, len);
    } else {
        DEBUG_WARN ("Error queuing MQTT %s", topic);
    }
    return result;
}
#endif


bool GwOutput_MQTT::outputDataSend (char* address, char* data, size_t length, GwOutput_data_type_t type) {
	const int TOPIC_SIZE = 64;
	char topic[TOPIC_SIZE];
	bool result;
	switch (type) {
	case GwOutput_data_type::data:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, NODE_DATA);
		break;
	case GwOutput_data_type::lostmessages:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, LOST_MESSAGES);
		break;
	case GwOutput_data_type::status:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, NODE_STATUS);
		break;
	}
	if ((result = addMQTTqueue (topic, data, length))) {
		DEBUG_INFO ("MQTT queued %s. Length %d", topic, length);
	} else {
		DEBUG_WARN ("Error queuing MQTT %s", topic);
	}
	return result;
}

bool GwOutput_MQTT::outputControlSend (char* address, uint8_t* data, size_t length) {
	const int TOPIC_SIZE = 64;
	const int PAYLOAD_SIZE = 512;
	char topic[TOPIC_SIZE];
	char payload[PAYLOAD_SIZE];
	size_t pld_size = 0;
	bool result = false;

	switch (data[0]) {
	case control_message_type::VERSION_ANS:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_VERSION_ANS);
		if (length >= 4) {
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"version\":\"%d.%d.%d\"}", data[1], data[2], data[3]);
		}
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
	case control_message_type::SLEEP_ANS:
		uint32_t sleepTime;
		memcpy (&sleepTime, data + 1, sizeof (sleepTime));
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_SLEEP_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"sleeptime\":%u}", sleepTime);
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
	case control_message_type::RESET_ANS:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, SET_RESET_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OK\"}");
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
    case control_message_type::RSSI_ANS:
        snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_RSSI_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"rssi\":%d,\"channel\":%u}", (int8_t)data[1], data[2]);
		EnigmaIOTGateway.getNodes ()->getNodeFromName (address)->setRSSI (data[1]);
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
	case control_message_type::NAME_ANS:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_NAME_ANS);
		char addrStr[ENIGMAIOT_ADDR_LEN * 3];
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"name\":\"%.*s\",\"address\":\"%s\"}", length - ENIGMAIOT_ADDR_LEN - 1, (char*)(data + 1 + ENIGMAIOT_ADDR_LEN), mac2str (data + 1, addrStr));
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
	case control_message_type::RESTART_CONFIRM:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, RESTART_NOTIF);
		if (length > 1) {
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"reason\":%d}", (int8_t)data[1]);
		}
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
	case control_message_type::OTA_ANS:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, SET_OTA_ANS);
		switch (data[1]) {
		case ota_status::OTA_STARTED:
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA Started\",\"status\":%u}", data[1]);
			break;
		case ota_status::OTA_START_ERROR:
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA Start error\",\"status\":%u}", data[1]);
			break;
		case ota_status::OTA_OUT_OF_SEQUENCE:
			uint16_t lastGoodIdx;
			memcpy ((uint8_t*)&lastGoodIdx, data + 2, sizeof (uint16_t));
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"last_chunk\":%d,\"result\":\"OTA out of sequence error\",\"status\":%u}", lastGoodIdx, data[1]);
			break;
		case ota_status::OTA_CHECK_OK:
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA check OK\",\"status\":%u}", data[1]);
			break;
		case ota_status::OTA_CHECK_FAIL:
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA check failed\",\"status\":%u}", data[1]);
			break;
		case ota_status::OTA_TIMEOUT:
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA timeout\",\"status\":%u}", data[1]);
			break;
		case ota_status::OTA_FINISHED:
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA finished OK\",\"status\":%u}", data[1]);
			break;
		}
		if (addMQTTqueue (topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
			result = true;
		}
		break;
	default:
		DEBUG_WARN ("Unknown control message. Code: 0x%02X", data[0]);
	}

	return result;
}

bool GwOutput_MQTT::newNodeSend (char* address, uint16_t node_id) {
	const int TOPIC_SIZE = 64;

	char topic[TOPIC_SIZE];

	uint8_t* nodeAddress = enigmaIotGateway->getNodes ()->getNodeFromID (node_id)->getMacAddress ();
	char addrStr[ENIGMAIOT_ADDR_LEN * 3];

	char payload[ENIGMAIOT_ADDR_LEN * 3 + 14];

	snprintf (payload, ENIGMAIOT_ADDR_LEN * 3 + 14, "{\"address\":\"%s\"}", mac2str (nodeAddress, addrStr));

	snprintf (topic, TOPIC_SIZE, "%s/%s/hello", netName.c_str (), address);
	bool result = addMQTTqueue (topic, payload, ENIGMAIOT_ADDR_LEN * 3 + 13);
	DEBUG_INFO ("Published MQTT %s", topic);
	return result;
}

bool GwOutput_MQTT::nodeDisconnectedSend (char* address, gwInvalidateReason_t reason) {
	const int TOPIC_SIZE = 64;
	const int PAYLOAD_SIZE = 64;

	char topic[TOPIC_SIZE];
	char payload[PAYLOAD_SIZE];
	size_t pld_size;

	snprintf (topic, TOPIC_SIZE, "%s/%s/bye", netName.c_str (), address);
	pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"reason\":%d}", reason);
	bool result = addMQTTqueue (topic, payload, pld_size);
	DEBUG_INFO ("Published MQTT %s result = %s", topic, result ? "OK" : "Fail");
	return result;
}