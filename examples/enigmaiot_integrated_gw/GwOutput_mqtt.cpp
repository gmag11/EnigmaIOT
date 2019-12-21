// 
// 
// 

#include "GwOutput_mqtt.h"
#include <ArduinoJson.h>

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

	DynamicJsonDocument doc (512);

	doc["mqtt_server"] = mqttgw_config.mqtt_server;
	doc["mqtt_port"] = mqttgw_config.mqtt_port;
	doc["mqtt_user"] = mqttgw_config.mqtt_user;
	doc["mqtt_pass"] = mqttgw_config.mqtt_pass;

	if (serializeJson (doc, configFile) == 0) {
		DEBUG_ERROR ("Failed to write to file");
		configFile.close ();
		//SPIFFS.remove (CONFIG_FILE);
		return false;
	}

	String output;
	serializeJsonPretty (doc, output);

	DEBUG_DBG ("%s", output.c_str ());

	configFile.flush ();
	size_t size = configFile.size ();

	//configFile.write ((uint8_t*)(&mqttgw_config), sizeof (mqttgw_config));
	configFile.close ();
	DEBUG_DBG ("Gateway configuration saved to flash. %u bytes", size);
	return true;
}

bool GwOutput_MQTT::loadConfig () {
	//SPIFFS.remove (CONFIG_FILE); // Only for testing
	bool json_correct = false;

	if (!SPIFFS.begin ()) {
		DEBUG_WARN ("Error starting filesystem. Formatting");
		SPIFFS.format ();
		WiFi.disconnect ();
	}

	if (SPIFFS.exists (CONFIG_FILE)) {

		DEBUG_DBG ("Opening %s file", CONFIG_FILE);
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
			size_t size = configFile.size ();
			DEBUG_DBG ("%s opened. %u bytes", CONFIG_FILE, size);
			/*if (size < sizeof (mqttgw_config_t)) {
				DEBUG_WARN ("Config file is corrupted. Deleting");
				SPIFFS.remove (CONFIG_FILE);
				return false;
			}*/
			DynamicJsonDocument doc (512);
			DeserializationError error = deserializeJson (doc, configFile);
			if (error) {
				DEBUG_ERROR ("Failed to parse file");
			} else {
				DEBUG_DBG ("JSON file parsed");
				//json_correct = true;
			}

			if (doc.containsKey ("mqtt_server") && doc.containsKey ("mqtt_port")
				&& doc.containsKey ("mqtt_user") && doc.containsKey ("mqtt_pass")) {
				json_correct = true;
			}

			//configFile.read ((uint8_t*)(&mqttgw_config), sizeof (mqttgw_config_t));

			strlcpy (mqttgw_config.mqtt_server, doc["mqtt_server"] | "", sizeof (mqttgw_config.mqtt_server));
			mqttgw_config.mqtt_port = doc["mqtt_port"].as<int> ();
			strlcpy (mqttgw_config.mqtt_user, doc["mqtt_user"] | "", sizeof (mqttgw_config.mqtt_user));
			strlcpy (mqttgw_config.mqtt_pass, doc["mqtt_pass"] | "", sizeof (mqttgw_config.mqtt_pass));


			configFile.close ();
			if (json_correct) {
				DEBUG_INFO ("Gateway configuration successfuly read");
			}
			DEBUG_DBG (		"==== MQTT Configuration ====");
			DEBUG_DBG (		"MQTT server: %s", mqttgw_config.mqtt_server);
			DEBUG_DBG (		"MQTT port: %d", mqttgw_config.mqtt_port);
			DEBUG_DBG (		"MQTT user: %s", mqttgw_config.mqtt_user);
			DEBUG_VERBOSE (	"MQTT password: %s", mqttgw_config.mqtt_pass);

			String output;
			serializeJsonPretty (doc, output);

			DEBUG_DBG ("JSON file %s", output.c_str ());

			//return json_correct;
		} else {
			DEBUG_WARN ("Error opening %s", CONFIG_FILE);
		}
	} else {
		DEBUG_WARN ("%s do not exist", CONFIG_FILE);
		//return false;
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

	free (mqttServerParam);
	free (mqttPortParam);
	free (mqttUserParam);
	free (mqttPassParam);
}

bool GwOutput_MQTT::begin () {

#ifdef ESP32
	esp_err_t err;
#endif
#ifdef SECURE_MQTT
#ifdef ESP32
	err = esp_tls_set_global_ca_store ((const unsigned char*)DSTroot_CA, sizeof (DSTroot_CA));
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
	gwTopic = netName + GW_STATUS;
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
	mqtt_cfg.lwt_topic = gwTopic.c_str ();
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

#ifdef ESP8266
void GwOutput_MQTT::reconnect () {
	// Loop until we're reconnected
	while (!client.connected ()) {
		// TODO: startConnectionFlash (500);
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
			publishMQTT (this, gwTopic.c_str (), "1", 1, true);
			// ... and resubscribe
			String dlTopic = netName + String ("/+/set/#");
			client.subscribe (dlTopic.c_str ());
			dlTopic = netName + String ("/+/get/#");
			client.subscribe (dlTopic.c_str ());
			client.setCallback (onDlData);
			// TODO: stopConnectionFlash ();
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
esp_err_t GwOutput_MQTT::mqtt_event_handler (esp_mqtt_event_handle_t event) {
	if (event->event_id == MQTT_EVENT_CONNECTED) {
		DEBUG_DBG ("MQTT msgid= %d event: %d. MQTT_EVENT_CONNECTED", event->msg_id, event->event_id);
		String topic = GwOutput.netName + "/+/set/#";
		esp_mqtt_client_subscribe (GwOutput.client, topic.c_str(), 0);
		topic = GwOutput.netName + "/+/get/#";
		esp_mqtt_client_subscribe (GwOutput.client, topic.c_str (), 0);
		//esp_mqtt_client_publish (client, "test/status", "1", 1, 0, false);
		publishMQTT (&GwOutput, (char*)GwOutput.gwTopic.c_str (), "1", 1, true);
	} else if (event->event_id == MQTT_EVENT_DISCONNECTED) {
		DEBUG_WARN ("MQTT event: %d. MQTT_EVENT_DISCONNECTED", event->event_id);
		//esp_mqtt_client_reconnect (event->client); //not needed if autoconnect is enabled
	} else  if (event->event_id == MQTT_EVENT_SUBSCRIBED) {
		DEBUG_DBG ("MQTT msgid= %d event: %d. MQTT_EVENT_SUBSCRIBED", event->msg_id, event->event_id);
	} else  if (event->event_id == MQTT_EVENT_UNSUBSCRIBED) {
		DEBUG_DBG ("MQTT msgid= %d event: %d. MQTT_EVENT_UNSUBSCRIBED", event->msg_id, event->event_id);
	} else  if (event->event_id == MQTT_EVENT_PUBLISHED) {
		DEBUG_DBG ("MQTT event: %d. MQTT_EVENT_PUBLISHED", event->event_id);
	} else  if (event->event_id == MQTT_EVENT_DATA) {
		DEBUG_DBG ("MQTT msgid= %d event: %d. MQTT_EVENT_DATA", event->msg_id, event->event_id);
		DEBUG_DBG ("Topic length %d. Total Data length %d. Data length %d", event->topic_len, event->total_data_len, event->data_len);
		DEBUG_DBG ("Incoming data: %.*s %.*s", event->topic_len, event->topic, event->data_len, event->data);
		// Reject big packets: total_data_len > data_len
		// Reject continuations: topic_len = 0
		if (event->total_data_len == event->data_len && event->topic_len > 0) {
			char* topic = (char*)malloc (event->topic_len + 1);
			snprintf (topic, event->topic_len + 1, "%.*s", event->topic_len, event->topic);
			DEBUG_VERBOSE ("Built Topic %s", topic);
			onDlData (topic, (uint8_t*)event->data, event->data_len);
			free (topic);
		}

	} else  if (event->event_id == MQTT_EVENT_BEFORE_CONNECT) {
		DEBUG_DBG ("MQTT event: %d. MQTT_EVENT_BEFORE_CONNECT", event->event_id);
	}
}
#endif // ESP32

char* getTopicAddress (char* topic, unsigned int &len){
	if (!topic) 
		return NULL;

	char* start = strchr (topic, '/') + 1;
	char* end;
	
	if (start) {
		end = strchr (start, '/');
	} else {
		return NULL;
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
	if        (data == GET_VERSION) {
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
	} else
		return control_message_type::INVALID;
}

control_message_type_t getTopicType (char* topic, char* &userCommand) {
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
	if ((int)start > 0x01) {
		command = String(start);
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
	uint8_t addr[6];
	char* addressStr;
	control_message_type_t msgType;
	char* userCommand;


	DEBUG_DBG ("Topic %s", topic);

	unsigned int addressLen;

	addressStr = getTopicAddress (topic, addressLen);
	//DEBUG_DBG ("addressStr %p : %d", addressStr, addressLen);

	if (addressStr) {
		//DEBUG_INFO ("Len: %u", addressLen);
		DEBUG_DBG ("Address %.*s", addressLen, addressStr);
		if (!str2mac (addressStr, addr)) {
			DEBUG_DBG ("Not a mac address");
			return;
		}
		DEBUG_DBG ("Hex Address = %s", printHexBuffer (addr, 6));
	} else
		return;

	msgType = getTopicType (topic, userCommand);


	DEBUG_DBG ("User command: %s", userCommand);
	DEBUG_DBG ("MsgType 0x%02X", msgType);
	DEBUG_DBG ("Data: %.*s\n", len, data);

	if (msgType != control_message_type_t::INVALID)
		GwOutput.downlinkCb (addr, msgType, (char*)data, len);
	else
		DEBUG_DBG ("Invalid message");
}

void GwOutput_MQTT::loop () {
#ifdef ESP8266
	client.loop ();
	if (!client.connected ()) {
		DEBUG_INFO ("reconnect");
		reconnect ();
	}
#endif
}

bool GwOutput_MQTT::publishMQTT (GwOutput_MQTT* gw, const char* topic, char* payload, size_t len, bool retain) {
#ifdef ESP32
	int result = esp_mqtt_client_publish (gw->client, topic, payload, len, 0, retain);
	DEBUG_INFO ("-----> MQTT result %d", result);
	return true; // TODO: FIX --> result;
#elif defined(ESP8266)
	return gw->client.publish (topic, (uint8_t*)payload, len, retain);
#endif // ESP32
}

#ifdef SECURE_MQTT
void GwOutput_MQTT::setClock () {
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

bool GwOutput_MQTT::outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type) {
	const int TOPIC_SIZE = 64;
	char* topic = (char*)malloc (TOPIC_SIZE);
	bool result;
	switch (type){
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
	if (result = publishMQTT (this, topic, data, length)) {
		DEBUG_INFO ("Published MQTT %s", topic);
	} else {
		DEBUG_WARN ("Error publishing MQTT %s", topic);
	}
	free (topic);
	return result;
}

bool GwOutput_MQTT::outputControlSend (char* address, uint8_t* data, uint8_t length) {
	const int TOPIC_SIZE = 64;
	const int PAYLOAD_SIZE = 512;
	char* topic = (char*)malloc (TOPIC_SIZE);
	char* payload = (char*)malloc (PAYLOAD_SIZE);
	size_t pld_size;

	switch (data[0]) {
	case control_message_type::VERSION_ANS:
		//Serial.printf ("~/%s/%s;{\"version\":\"", address, GET_VERSION_ANS);
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_VERSION_ANS);
		//Serial.write (data + 1, length - 1);
		//Serial.println ("\"}");
		//Serial.printf ("%s/%s/%s;{\"version\":\"", EnigmaIOTGateway.getNetworkName (), address, GET_VERSION_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"version\":\"%.*s\"}", length - 1, data + 1);
		if (publishMQTT (this, topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
		}
		break;
	case control_message_type::SLEEP_ANS:
		uint32_t sleepTime;
		memcpy (&sleepTime, data + 1, sizeof (sleepTime));
		//Serial.printf ("~/%s/%s;{\"sleeptime\":%d}\n", address, GET_SLEEP_ANS, sleepTime);
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_SLEEP_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"sleeptime\":%d}", sleepTime);
		if (publishMQTT (this, topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
		}
		break;
	case control_message_type::RESET_ANS:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, SET_RESET_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{}");
		if (publishMQTT (this, topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
		}
		break;
	case control_message_type::RSSI_ANS:
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str (), address, GET_RSSI_ANS);
		pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"rssi\":%d,\"channel\":%u}", (int8_t)data[1], data[2]);
		if (publishMQTT (this, topic, payload, pld_size)) {
			DEBUG_INFO ("Published MQTT %s %s", topic, payload);
		}
		break;
	case control_message_type::OTA_ANS:
		//Serial.printf ("~/%s/%s;", address, SET_OTA_ANS);
		snprintf (topic, TOPIC_SIZE, "%s/%s/%s", netName.c_str(), address, SET_OTA_ANS);
		switch (data[1]) {
		case ota_status::OTA_STARTED:
			//Serial.printf ("{\"result\":\"OTA Started\",\"status\":%u}\n", data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA Started\",\"status\":%u}\n", data[1]);
			break;
		case ota_status::OTA_START_ERROR:
			//Serial.printf ("{\"result\":\"OTA Start error\",\"status\":%u}\n", data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA Start error\",\"status\":%u}\n", data[1]);
			break;
		case ota_status::OTA_OUT_OF_SEQUENCE:
			uint16_t lastGoodIdx;
			memcpy ((uint8_t*)&lastGoodIdx, data + 2, sizeof (uint16_t));
			//Serial.printf ("{\"last_chunk\":%d,\"result\":\"OTA out of sequence error\",\"status\":%u}\n", lastGoodIdx, data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"last_chunk\":%d,\"result\":\"OTA out of sequence error\",\"status\":%u}\n", lastGoodIdx, data[1]);
			break;
		case ota_status::OTA_CHECK_OK:
			//Serial.printf ("{\"result\":\"OTA check OK\",\"status\":%u}\n", data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA check OK\",\"status\":%u}\n", data[1]);
			break;
		case ota_status::OTA_CHECK_FAIL:
			//Serial.printf ("{\"result\":\"OTA check failed\",\"status\":%u}\n", data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA check failed\",\"status\":%u}\n", data[1]);
			break;
		case ota_status::OTA_TIMEOUT:
			//Serial.printf ("{\"result\":\"OTA timeout\",\"status\":%u}\n", data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA timeout\",\"status\":%u}\n", data[1]);
			break;
		case ota_status::OTA_FINISHED:
			//Serial.printf ("{\"result\":\"OTA finished OK\",\"status\":%u}\n", data[1]);
			pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"result\":\"OTA finished OK\",\"status\":%u}\n", data[1]);
			break;
		//default:
			//Serial.println ();
		}
		break;
	}

	free (topic);
	free (payload);

}

bool GwOutput_MQTT::newNodeSend (char* address) {
	const int TOPIC_SIZE = 64;
	char* topic = (char*)malloc (TOPIC_SIZE);
	snprintf (topic, TOPIC_SIZE, "%s/%s/hello", netName.c_str(), address);
	bool result = publishMQTT (this, topic, "", 0);
	DEBUG_INFO ("Published MQTT %s", topic);
	free (topic);
	return result;
}

bool GwOutput_MQTT::nodeDisconnectedSend (char* address, gwInvalidateReason_t reason) {
	const int TOPIC_SIZE = 64;
	const int PAYLOAD_SIZE = 64;
	char* topic = (char*)malloc (TOPIC_SIZE);
	char* payload = (char*)malloc (PAYLOAD_SIZE);
	size_t pld_size;

	snprintf (topic, TOPIC_SIZE, "%s/%s/bye", netName.c_str(), address);
	pld_size = snprintf (payload, PAYLOAD_SIZE, "{\"reason\":%u}", reason);
	bool result = publishMQTT (this, topic, payload, pld_size);
	DEBUG_INFO ("Published MQTT %s", topic);
	free (topic);
	free (payload);
	return result;
}