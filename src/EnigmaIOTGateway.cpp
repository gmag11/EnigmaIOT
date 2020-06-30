/**
  * @file EnigmaIOTGateway.cpp
  * @version 0.9.2
  * @date 01/07/2020
  * @author German Martin
  * @brief Library to build a gateway for EnigmaIoT system
  */

#include "EnigmaIOTGateway.h"
#include <FS.h>
#include "libb64/cdecode.h"
#include <ArduinoJson.h>
#ifdef ESP8266
#include <Updater.h>
#elif defined ESP32
#include <SPIFFS.h>
#include <Update.h>
#include <esp_wifi.h>
#endif

#include "cryptModule.h"
#include "helperFunctions.h"
#include <cstddef>
#include <cstdint>


const char CONFIG_FILE[] = "/config.json";

bool shouldSave = false;
bool OTAongoing = false;
time_t lastOTAmsg = 0;


void EnigmaIOTGatewayClass::doSave (void) {
	DEBUG_INFO ("Configuration saving activated");
	shouldSave = true;
}

bool EnigmaIOTGatewayClass::getShouldSave () {
	return (shouldSave);
}

void EnigmaIOTGatewayClass::setTxLed (uint8_t led, time_t onTime) {
	this->txled = led;
	txLedOnTime = onTime;
	pinMode (txled, OUTPUT);
	digitalWrite (txled, HIGH);
}

void EnigmaIOTGatewayClass::setRxLed (uint8_t led, time_t onTime) {
	this->rxled = led;
	rxLedOnTime = onTime;
	pinMode (rxled, OUTPUT);
	digitalWrite (rxled, HIGH);
}

const void* memstr (const void* str, size_t str_size,
					const char* target, size_t target_size) {
	const uint8_t* pointer = (const uint8_t*)str;
	for (size_t i = 0; i != str_size - target_size; ++i) {
		if (!memcmp (pointer + i, target, target_size)) {
			return pointer + i;
		}
	}

	return NULL;
}

bool buildGetVersion (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_DBG ("Build 'Get Version' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 1) {
		return false;
	}
	data[0] = (uint8_t)control_message_type::VERSION;
	dataLen = 1;
	return true;
}

bool buildGetSleep (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Get Sleep' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 1) {
		return false;
	}
	data[0] = (uint8_t)control_message_type::SLEEP_GET;
	dataLen = 1;
	return true;
}

bool buildSetIdentify (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Set Identify' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 1) {
		return false;
	}
	data[0] = (uint8_t)control_message_type::IDENTIFY;
	dataLen = 1;
	return true;
}

bool buildGetRSSI (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Get RSSI' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 1) {
		return false;
	}
	data[0] = (uint8_t)control_message_type::RSSI_GET;
	dataLen = 1;
	return true;
}

bool buildGetName (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Get Node Name and Address' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 1) {
		return false;
	}
	data[0] = (uint8_t)control_message_type::NAME_GET;
	dataLen = 1;
	return true;
}

bool buildSetName (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Set Node Name' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < NODE_NAME_LENGTH + 1) {
		DEBUG_ERROR ("Not enough space to build message");
		return false;
	}
	if (inputLen < 2 || inputLen > NODE_NAME_LENGTH) {
		DEBUG_ERROR ("Name too short");
		return false;
	}
	data[0] = (uint8_t)control_message_type::NAME_SET;
	memcpy (data + 1, inputData, inputLen);
	dataLen = 1 + inputLen;
	return true;
}

bool buildSetResetConfig (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Reset Config' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 1) {
		return false;
	}
	data[0] = (uint8_t)control_message_type::RESET;
	dataLen = 1;
	return true;
}

int getNextNumber (char*& data, size_t& len/*, char* &position*/) {
	char strNum[10];
	int number;
	char* tempData = data;
	size_t tempLen = len;

	for (int i = 0; i < 10; i++) {
		//DEBUG_DBG ("Processing char: %c", tempData[i]);
		if (tempData[i] != ',') {
			if (tempData[i] >= '0' && tempData[i] <= '9') {
				strNum[i] = tempData[i];
			} else {
				DEBUG_ERROR ("OTA message format error. Message number not found");
				number = -1;
			}
			if (i == 9) {
				DEBUG_ERROR ("OTA message format error, separator not found");
				number = -2;
			}
		} else {
			if (i == 0) {
				DEBUG_ERROR ("OTA message format error, cannot find a number");
				number = -3;
			}
			strNum[i] = '\0';
			//DEBUG_DBG ("Increment pointer by %d", i);
			tempData += i;
			tempLen -= i;
			break;
		}
	}
	if (tempData[0] == ',' && tempLen > 0) {
		tempData++;
		tempLen--;
	} else {
		DEBUG_WARN ("OTA message format warning. separator not found");
	}
	number = atoi (strNum);
	data = tempData;
	len = tempLen;
	DEBUG_DBG ("Extracted number %d", number);
	DEBUG_DBG ("Resulting data %s", data);
	//DEBUG_WARN ("Resulting length %d", len);
	return number;
}

bool isHexChar (char c) {
	//DEBUG_DBG ("Is Hex Char %c", c);
	return (
		(c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'f')
		//|| c >= 'A' && c <= 'F'
		);
}

bool buildOtaMsg (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	char* payload;
	size_t payloadLen;
	int number;
	uint8_t* tempData = data;

	DEBUG_VERBOSE ("Build 'OTA' message from: %s", inputData);

	payload = (char*)inputData;
	payloadLen = inputLen;

	// Get message number
	number = getNextNumber (payload, payloadLen);
	if (number < 0) {
		return false;
	}
	uint16_t msgIdx = number;

	tempData[0] = (uint8_t)control_message_type::OTA;
	tempData++;
	memcpy (tempData, &msgIdx, sizeof (uint16_t));
	size_t decodedLen = sizeof (uint8_t) + sizeof (uint16_t);
	tempData += sizeof (uint16_t);

	DEBUG_WARN ("OTA message number %u", msgIdx);
	//DEBUG_INFO ("Payload len = %u", payloadLen);
	//DEBUG_INFO ("Payload data: %s", payload);

	if (msgIdx > 0) {
		decodedLen += base64_decode_chars (payload, payloadLen, (char*)(data + 1 + sizeof (uint16_t)));
		lastOTAmsg = millis ();
	} else {
		OTAongoing = true;
		lastOTAmsg = millis ();

		if (inputLen < 39) {
			DEBUG_ERROR ("OTA message format error. Message #0 too short to be a MD5 string");
			return false;
		}

		// Get firmware size
		number = getNextNumber (payload, payloadLen);
		if (number < 0) {
			return false;
		}
		uint32_t fileSize = number;

		memcpy (tempData, &fileSize, sizeof (uint32_t));
		tempData += sizeof (uint32_t);
		decodedLen += sizeof (uint32_t);


		// Get number of chunks
		number = getNextNumber (payload, payloadLen);
		if (number < 0) {
			return false;
		}
		uint16_t msgNum = number;

		memcpy (tempData, &msgNum, sizeof (uint16_t));
		tempData += sizeof (uint16_t);
		decodedLen += sizeof (uint16_t);

		DEBUG_WARN ("Number of OTA chunks %u", msgNum);
		DEBUG_WARN ("OTA length = %u bytes", fileSize);
		//DEBUG_INFO ("Payload data: %s", payload);

		if (payloadLen < 32) {
			DEBUG_ERROR ("OTA message format error. MD5 is too short: %d", payloadLen);
			return false;
		}

		for (size_t i = 0; i < 32; i++) {
			if (!isHexChar (payload[i])) {
				DEBUG_ERROR ("OTA message format error. MD5 string has no valid format");
				return false;
			}
			*tempData = (uint8_t)payload[i];
			tempData++;
			decodedLen++;
		}

		DEBUG_VERBOSE ("Payload data: %s", printHexBuffer (data, decodedLen));
	}

	if ((decodedLen) > MAX_MESSAGE_LENGTH) {
		DEBUG_ERROR ("OTA message too long. %u bytes.", decodedLen);
		return false;
	}
	dataLen = decodedLen;
	DEBUG_VERBOSE ("Payload has %u bytes of data: %s", dataLen, printHexBuffer (data, dataLen));
	return true;
}

bool buildSetSleep (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Set Sleep' message from: %s", printHexBuffer (inputData, inputLen));
	if (dataLen < 5) {
		DEBUG_ERROR ("Not enough space to build message");
		return false;
	}

	if (inputLen <= 1) {
		DEBUG_ERROR ("Set sleep time value is empty");
		return false;
	}

	for (unsigned int i = 0; i < (inputLen - 1); i++) { // Check if all digits are number
		if (inputData[i] < 30 || inputData[i] > '9') {
			DEBUG_ERROR ("Set sleep time value is not a number on position %d: %d", i, inputData[i]);
			return false;
		}
	}
	if (inputData[inputLen - 1] != 0) { // Array should end with \0
		DEBUG_ERROR ("Set sleep time value does not end with \\0");
		return false;
	}

	uint32_t sleepTime = atoi ((char*)inputData);

	data[0] = (uint8_t)control_message_type::SLEEP_SET;
	memcpy (data + 1, &sleepTime, sizeof (uint32_t));
	dataLen = 5;
	return true;
}

bool EnigmaIOTGatewayClass::sendDownstream (uint8_t* mac, const uint8_t* data, size_t len, control_message_type_t controlData, gatewayPayloadEncoding_t encoding, char* nodeName) {
	Node* node;
	if (nodeName) {
		node = nodelist.getNodeFromName (nodeName);
		if (node) {
			uint8_t* addr = node->getMacAddress ();
			DEBUG_DBG ("Message to node %s with address " MACSTR, nodeName, MAC2STR (addr));
		}
		//DEBUG_WARN ("Message to node %s with address %s", nodeName, mac2str (node->getMacAddress (), addrStr));
	} else {
		node = nodelist.getNodeFromMAC (mac);
	}

	uint8_t downstreamData[MAX_MESSAGE_LENGTH];

	if (len == 0 && (controlData == USERDATA_GET || controlData == USERDATA_SET))
		return false;

	DEBUG_VERBOSE ("Downstream: %s", printHexBuffer (data, len));
	DEBUG_DBG ("Downstream message type 0x%02X", controlData);

	size_t dataLen = MAX_MESSAGE_LENGTH;

	switch (controlData) {
	case control_message_type::VERSION:
		if (!buildGetVersion (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building get Version message");
			return false;
		}
		DEBUG_VERBOSE ("Get Version. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::SLEEP_GET:
		if (!buildGetSleep (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building get Sleep message");
			return false;
		}
		DEBUG_VERBOSE ("Get Sleep. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::SLEEP_SET:
		if (!buildSetSleep (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building set Sleep message");
			return false;
		}
		DEBUG_VERBOSE ("Set Sleep. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::OTA:
		if (!buildOtaMsg (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building OTA message");
			return false;
		}
		DEBUG_VERBOSE ("OTA message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::IDENTIFY:
		if (!buildSetIdentify (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building Identify message");
			return false;
		}
		DEBUG_VERBOSE ("Identify message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::RESET:
		if (!buildSetResetConfig (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building Reset message");
			return false;
		}
		DEBUG_VERBOSE ("Reset Config message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::RSSI_GET:
		if (!buildGetRSSI (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building get RSSI message");
			return false;
		}
		DEBUG_VERBOSE ("Get RSSI message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::NAME_GET:
		if (!buildGetName (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building get name message");
			return false;
		}
		DEBUG_VERBOSE ("Get name message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::NAME_SET:
		if (!buildSetName (downstreamData, dataLen, data, len)) {
			DEBUG_ERROR ("Error building set name message");
			return false;
		}
		DEBUG_VERBOSE ("Set name message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	case control_message_type::USERDATA_GET:
		DEBUG_INFO ("Data message GET");
		break;
	case control_message_type::USERDATA_SET:
		DEBUG_INFO ("Data message SET");
		break;
	default:
		return false;
	}


	DEBUG_INFO ("Send downstream");

	if (node) {
		if (controlData != control_message_type::USERDATA_GET && controlData != control_message_type::USERDATA_SET)
			return downstreamDataMessage (node, downstreamData, dataLen, controlData);
		else if (controlData == control_message_type::OTA) {
			if (node->getSleepy ()) {
				DEBUG_ERROR ("Node must be in non sleepy mode to receive OTA messages");
				return false;
			} else
				return downstreamDataMessage (node, data, len, controlData);
		} else
			return downstreamDataMessage (node, data, len, controlData, encoding);
	} else {
		char addr[ENIGMAIOT_ADDR_LEN * 3];
		DEBUG_ERROR ("Downlink destination %s not found", nodeName ? nodeName : mac2str (mac, addr));
		return false;
	}
}

bool EnigmaIOTGatewayClass::configWiFiManager () {
	server = new AsyncWebServer (80);
	dns = new DNSServer ();
	wifiManager = new AsyncWiFiManager (server, dns);

	char networkKey[33] = "";
	//char networkName[NETWORK_NAME_LENGTH] = "";
	char channel[4];
	String (gwConfig.channel).toCharArray (channel, 4);

	//AsyncWiFiManager wifiManager (&server, &dns);
	AsyncWiFiManagerParameter netNameParam ("netname", "Network Name", gwConfig.networkName, (int)NETWORK_NAME_LENGTH - 1, "required type=\"text\" maxlength=20");
	AsyncWiFiManagerParameter netKeyParam ("netkey", "NetworkKey", networkKey, 33, "required type=\"password\" maxlength=32");
	AsyncWiFiManagerParameter channelParam ("channel", "WiFi Channel", channel, 4, "required type=\"number\" min=\"0\" max=\"13\" step=\"1\"");

	wifiManager->addParameter (&netKeyParam);
	wifiManager->addParameter (&channelParam);
	wifiManager->addParameter (&netNameParam);
	wifiManager->addParameter (new AsyncWiFiManagerParameter ("<br>"));

	if (notifyWiFiManagerStarted) {
		notifyWiFiManagerStarted ();
	}

	wifiManager->setDebugOutput (true);
#if CONNECT_TO_WIFI_AP != 1
	wifiManager->setBreakAfterConfig (true);
#endif // CONNECT_TO_WIFI_AP
	wifiManager->setTryConnectDuringConfigPortal (false);
	wifiManager->setSaveConfigCallback (doSave);
	wifiManager->setConfigPortalTimeout (150);

#if CONNECT_TO_WIFI_AP == 1
	boolean result = wifiManager->autoConnect ("EnigmaIoTGateway", NULL, 3, 2000);
#else
	boolean result = wifiManager->startConfigPortal ("EnigmaIoTGateway", NULL);
	result = true; // Force true if this should not connect to a WiFi
#endif // CONNECT_TO_WIFI_AP

	DEBUG_INFO ("==== Config Portal result ====");
	DEBUG_INFO ("Network Name: %s", netNameParam.getValue ());
	DEBUG_INFO ("Network Key: %s", netKeyParam.getValue ());
	DEBUG_INFO ("Channel: %s", channelParam.getValue ());
	DEBUG_INFO ("Status: %s", result ? "true" : "false");
	DEBUG_INFO ("Save config: %s", shouldSave ? "yes" : "no");
	if (result) {
		if (shouldSave) {
			memcpy (gwConfig.networkName, netNameParam.getValue (), netNameParam.getValueLength ());
			uint8_t keySize = netKeyParam.getValueLength ();
			if (keySize > KEY_LENGTH)
				keySize = KEY_LENGTH;
			const char* netKey = netKeyParam.getValue ();
			if (netKey && (netKey[0] != '\0')) {// If password is empty, keep the old one
				memset (this->gwConfig.networkKey, 0, KEY_LENGTH);
				memcpy (this->gwConfig.networkKey, netKey, keySize);
				memcpy (this->networkKey, netKey, keySize);
				CryptModule::getSHA256 (this->gwConfig.networkKey, KEY_LENGTH);
			} else {
				DEBUG_INFO ("Network key password field empty. Keeping the old one");
			}

			gwConfig.channel = atoi (channelParam.getValue ());
			DEBUG_DBG ("Raw network Key: %s", printHexBuffer (this->gwConfig.networkKey, KEY_LENGTH));
			DEBUG_VERBOSE ("WiFi ESP-NOW channel: %d", this->gwConfig.channel);
		} else {
			DEBUG_DBG ("Configuration does not need to be saved");
		}
	} else {
		DEBUG_ERROR ("WiFi connection unsuccessful. Restarting");
		ESP.restart ();
	}

	if (notifyWiFiManagerExit) {
		notifyWiFiManagerExit (result);
	}

	delete (server);
	delete (dns);
	delete (wifiManager);

	return result;
}

bool EnigmaIOTGatewayClass::loadFlashData () {
	//SPIFFS.remove (CONFIG_FILE); // Only for testing
	bool json_correct = false;

	if (SPIFFS.exists (CONFIG_FILE)) {

		DEBUG_DBG ("Opening %s file", CONFIG_FILE);
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
			size_t size = configFile.size ();
			DEBUG_DBG ("%s opened. %u bytes", CONFIG_FILE, size);

			const size_t capacity = JSON_OBJECT_SIZE (3) + 50;
			DynamicJsonDocument doc (capacity);

			DeserializationError error = deserializeJson (doc, configFile);
			if (error) {
				DEBUG_ERROR ("Failed to parse file");
			} else {
				DEBUG_DBG ("JSON file parsed");
			}

			if (doc.containsKey ("channel") && doc.containsKey ("networkKey")
				&& doc.containsKey ("networkName")) {
				json_correct = true;
			}

			gwConfig.channel = doc["channel"].as<int> ();
			strncpy ((char*)gwConfig.networkKey, doc["networkKey"] | "", sizeof (gwConfig.networkKey));
			strncpy (gwConfig.networkName, doc["networkName"] | "", sizeof (gwConfig.networkName));

			configFile.close ();
			if (json_correct) {
				DEBUG_VERBOSE ("Gateway configuration successfuly read");
			}
			DEBUG_DBG ("==== EnigmaIOT Gateway Configuration ====");
			DEBUG_DBG ("Network name: %s", gwConfig.networkName);
			DEBUG_DBG ("WiFi channel: %u", gwConfig.channel);
			DEBUG_VERBOSE ("Network key: %s", gwConfig.networkKey);
			strncpy (plainNetKey, (char*)gwConfig.networkKey, KEY_LENGTH);
			CryptModule::getSHA256 (gwConfig.networkKey, KEY_LENGTH);
			DEBUG_VERBOSE ("Raw Network key: %s", printHexBuffer (gwConfig.networkKey, KEY_LENGTH));

			String output;
			serializeJsonPretty (doc, output);

			DEBUG_DBG ("JSON file %s", output.c_str ());

		} else {
			DEBUG_WARN ("Error opening %s", CONFIG_FILE);
		}
	} else {
		DEBUG_WARN ("%s do not exist", CONFIG_FILE);
		//SPIFFS.format (); // Testing only
		//WiFi.begin ("0", "0"); // Delete WiFi credentials
		//DEBUG_WARN ("Dummy STA config loaded");
		//return false;
	}

	if (!json_correct) {
		WiFi.begin ("0", "0"); // Delete WiFi credentials
		DEBUG_WARN ("Dummy STA config loaded");
	}
	return json_correct;
}

bool EnigmaIOTGatewayClass::saveFlashData () {
	File configFile = SPIFFS.open (CONFIG_FILE, "w");
	if (!configFile) {
		DEBUG_WARN ("failed to open config file %s for writing", CONFIG_FILE);
		return false;
	}

	const size_t capacity = JSON_OBJECT_SIZE (3) + 50;
	DynamicJsonDocument doc (capacity);

	doc["channel"] = gwConfig.channel;
	doc["networkKey"] = networkKey;
	doc["networkName"] = gwConfig.networkName;

	if (serializeJson (doc, configFile) == 0) {
		DEBUG_ERROR ("Failed to write to file");
		configFile.close ();
		//SPIFFS.remove (CONFIG_FILE); // Testing only
		return false;
	}

	String output;
	serializeJsonPretty (doc, output);

	DEBUG_DBG ("%s", output.c_str ());

	configFile.flush ();
	size_t size = configFile.size ();

	configFile.close ();

	memset (networkKey, 0, KEY_LENGTH);

	DEBUG_DBG ("Gateway configuration saved to flash. %u bytes", size);
	return true;
}

void EnigmaIOTGatewayClass::begin (Comms_halClass* comm, uint8_t* networkKey, bool useDataCounter) {
	this->input_queue = new EnigmaIOTRingBuffer<msg_queue_item_t> (MAX_INPUT_QUEUE_SIZE);
	this->comm = comm;
	this->useCounter = useDataCounter;

	if (networkKey) {
		memcpy (this->gwConfig.networkKey, networkKey, KEY_LENGTH);
		strncpy (plainNetKey, (char*)networkKey, KEY_LENGTH);
		CryptModule::getSHA256 (this->gwConfig.networkKey, KEY_LENGTH);
	} else {
		if (!SPIFFS.begin ()) {
			DEBUG_ERROR ("Error mounting flash");
			SPIFFS.format ();
			DEBUG_ERROR ("Formatted");
			ESP.restart ();
			return;
		}
		if (!loadFlashData ()) { // Load from flash
			if (configWiFiManager ()) {
				if (shouldSave) {
					DEBUG_DBG ("Got configuration. Storing");
					if (saveFlashData ()) {
						DEBUG_DBG ("Network Key stored on flash");
					} else {
						DEBUG_ERROR ("Error saving data on flash");
					}
					ESP.restart ();
				} else {
					DEBUG_INFO ("Configuration has not to be saved");
				}
			} else {
				DEBUG_ERROR ("Configuration error. Restarting");
				ESP.restart ();
			}
		} else {
			DEBUG_INFO ("Configuration loaded from flash");
		}

		initWiFi (gwConfig.channel, COMM_GATEWAY, String (gwConfig.networkName));
		comm->begin (NULL, gwConfig.channel, COMM_GATEWAY);
		comm->onDataRcvd (rx_cb);
		comm->onDataSent (tx_cb);
	}
}

bool EnigmaIOTGatewayClass::addInputMsgQueue (const uint8_t* addr, const uint8_t* msg, size_t len) {
	msg_queue_item_t message;

	message.len = len;
	memcpy (message.data, msg, len);
	memcpy (message.addr, addr, ENIGMAIOT_ADDR_LEN);

#ifdef ESP32
	portENTER_CRITICAL (&myMutex);
#else
	noInterrupts ();
#endif
	input_queue->push (&message);
	char macstr[ENIGMAIOT_ADDR_LEN * 3];
	DEBUG_DBG ("Message 0x%02X added from %s. Size: %d", message.data[0], mac2str (message.addr, macstr), input_queue->size ());
#ifdef ESP32
	portEXIT_CRITICAL (&myMutex);
#else
	interrupts ();
#endif
	return true;
}

msg_queue_item_t* EnigmaIOTGatewayClass::getInputMsgQueue (msg_queue_item_t* buffer) {

	msg_queue_item_t* message;
#ifdef esp32
	portENTER_CRITICAL (&myMutex);
#else
	noInterrupts ();
#endif
	message = input_queue->front ();
	if (message) {
		DEBUG_DBG ("EnigmaIOT message got from queue. Size: %d", input_queue->size ());
		memcpy (buffer->data, message->data, message->len);
		memcpy (buffer->addr, message->addr, ENIGMAIOT_ADDR_LEN);
		buffer->len = message->len;
		popInputMsgQueue ();
	}
#ifdef esp32
	portEXIT_CRITICAL (&myMutex);
#else
	interrupts ();
#endif
	if (message) {
		return buffer;
	} else {
		return NULL;
	}
}

void EnigmaIOTGatewayClass::popInputMsgQueue () {
	if (input_queue->pop ()) {
		DEBUG_DBG ("EnigmaIOT message pop. Size %d", input_queue->size ());
	}
}

void EnigmaIOTGatewayClass::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {

	EnigmaIOTGateway.addInputMsgQueue (mac_addr, data, len);
}

void EnigmaIOTGatewayClass::tx_cb (uint8_t* mac_addr, uint8_t status) {
	EnigmaIOTGateway.getStatus (mac_addr, status);
}

void EnigmaIOTGatewayClass::getStatus (uint8_t* mac_addr, uint8_t status) {
	char buffer[ENIGMAIOT_ADDR_LEN * 3];
#ifdef ESP8266
	DEBUG_VERBOSE ("SENDStatus %s. Peer %s", status == 0 ? "OK" : "ERROR", mac2str (mac_addr, buffer));
#elif defined ESP32
	DEBUG_VERBOSE ("SENDStatus %d. Peer %s", status, mac2str (mac_addr, buffer));
#endif
}

void EnigmaIOTGatewayClass::handle () {
//#ifdef ESP8266
	static unsigned long rxOntime;
	static unsigned long txOntime;

	if (flashRx) {
		DEBUG_DBG ("EnigmaIOTGatewayClass::flashrx");

		if (rxled == txled) {
			flashTx = true;
		} else {
			rxOntime = millis ();
			digitalWrite (rxled, LOW);
		}
		flashRx = false;
	}

	if (rxled != txled) {
		if (!digitalRead (rxled) && millis () - rxOntime > rxLedOnTime) {
			digitalWrite (rxled, HIGH);
		}
	}

	if (flashTx) {
		txOntime = millis ();
		digitalWrite (txled, LOW);
		flashTx = false;
	}

	if (!digitalRead (txled) && millis () - txOntime > txLedOnTime) {
		digitalWrite (txled, HIGH);
	}
//#endif

	// Clean up dead nodes
	for (int i = 0; i < NUM_NODES; i++) {
		Node* node = nodelist.getNodeFromID (i);
		if (node->isRegistered () && millis () - node->getLastMessageTime () > MAX_NODE_INACTIVITY) {
			// TODO. Trigger node expired event
			node->reset ();
		}
	}

	if (OTAongoing) {
		time_t currentTime = millis ();
		if ((currentTime - lastOTAmsg) > OTA_GW_TIMEOUT) {
			OTAongoing = false;
			DEBUG_WARN ("OTA ongoing = false");
			DEBUG_WARN ("millis() = %u, lastOTAmsg = %u, diff = %d", currentTime, lastOTAmsg, currentTime - lastOTAmsg);
		}
	}

	// Check input EnigmaIOT message queue

	if (!input_queue->empty ()) {
		msg_queue_item_t* message;

		message = getInputMsgQueue (&tempBuffer);

		if (message) {
			DEBUG_DBG ("EnigmaIOT input message from queue. MsgType: 0x%02X", message->data[0]);
			manageMessage (message->addr, message->data, message->len);
		}
	}
}

void EnigmaIOTGatewayClass::manageMessage (const uint8_t* mac, uint8_t* buf, uint8_t count) {
	Node* node;

	DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	DEBUG_VERBOSE ("Received data: %s", printHexBuffer (buf, count));

	if (count <= 1) {
		DEBUG_WARN ("Empty message");
		return;
	}

	node = nodelist.getNewNode (mac);

	flashRx = true;

	int espNowError = 0; // May I remove this??

	switch (buf[0]) {
	case CLIENT_HELLO:
		// TODO: Do no accept new Client Hello if registration is on process on any node?? Possible DoS Attack??
		// May cause undesired behaviour in case a node registration message is lost
		DEBUG_INFO (" <------- CLIENT HELLO");
		//if (!OTAongoing) {
		if (espNowError == 0) {
			if (processClientHello (mac, buf, count, node)) {
				if (serverHello (myPublicKey, node)) {
					DEBUG_INFO ("Server Hello sent");
					node->setStatus (REGISTERED);
					node->setKeyValidFrom (millis ());
					node->setLastMessageCounter (0);
					node->setLastMessageTime ();
					if (notifyNewNode) {
						notifyNewNode (node->getMacAddress (), node->getNodeId (), NULL);
					}
#if DEBUG_LEVEL >= INFO
					nodelist.printToSerial (&DEBUG_ESP_PORT);
#endif
				} else {
					node->reset ();
					DEBUG_INFO ("Error sending Server Hello");
				}

			} else {
				// Ignore message in case of error
				//invalidateKey (node, WRONG_CLIENT_HELLO);
				node->reset ();
				DEBUG_ERROR ("Error processing client hello");
			}
		} else {
			DEBUG_ERROR ("Error adding peer %d", espNowError);
		}
		//} else {
		//	DEBUG_WARN ("OTA ongoing. Registration ignored");
		//}
		break;
	case CONTROL_DATA:
		DEBUG_INFO (" <------- CONTROL MESSAGE");
		if (node->getStatus () == REGISTERED) {
			if (processControlMessage (mac, buf, count, node)) {
				DEBUG_INFO ("Control message OK");
				if (millis () - node->getKeyValidFrom () > MAX_KEY_VALIDITY) {
					invalidateKey (node, KEY_EXPIRED);
				}
			} else {
				if (DISCONNECT_ON_DATA_ERROR) {
					invalidateKey (node, WRONG_DATA);
				}
				DEBUG_WARN ("Control message not OK");
			}
		} else {
			invalidateKey (node, UNREGISTERED_NODE);
		}
		break;
	case SENSOR_DATA:
	case UNENCRYPTED_NODE_DATA:
		bool encrypted;
		if (buf[0] == SENSOR_DATA) {
			DEBUG_INFO (" <------- ENCRYPTED DATA");
			encrypted = true;
		} else {
			DEBUG_INFO (" <------- UNENCRYPTED DATA");
			encrypted = false;
		}
		//if (!OTAongoing) {
		if (node->getStatus () == REGISTERED) {
			float packetsHour = (float)1 / ((millis () - node->getLastMessageTime ()) / (float)3600000);
			node->updatePacketsRate (packetsHour);
			if (processDataMessage (mac, buf, count, node, encrypted)) {
				node->setLastMessageTime ();
				DEBUG_INFO ("Data OK");
				DEBUG_VERBOSE ("Key valid from %lu ms", millis () - node->getKeyValidFrom ());
				if (millis () - node->getKeyValidFrom () > MAX_KEY_VALIDITY) {
					invalidateKey (node, KEY_EXPIRED);
				}
			} else {
				if (DISCONNECT_ON_DATA_ERROR) {
					invalidateKey (node, WRONG_DATA);
				}
				DEBUG_WARN ("Data not OK");
			}
		} else {
			invalidateKey (node, UNREGISTERED_NODE);
			node->reset ();
		}
		//} else {
		//	DEBUG_WARN ("Data ignored. OTA ongoing");
		//}
		break;
	case CLOCK_REQUEST:
		DEBUG_INFO (" <------- CLOCK REQUEST");
		if (node->getStatus () == REGISTERED) {
			if (processClockRequest (mac, buf, count, node)) {
				DEBUG_INFO ("Clock request OK");
				if (millis () - node->getKeyValidFrom () > MAX_KEY_VALIDITY) {
					invalidateKey (node, KEY_EXPIRED);
				}
			} else {
				invalidateKey (node, WRONG_DATA);
				DEBUG_WARN ("Clock request not OK");
			}

		} else {
			invalidateKey (node, UNREGISTERED_NODE);
		}
		break;
	case NODE_NAME_SET:
		DEBUG_INFO (" <------- NODE NAME REQUEST");
		if (node->getStatus () == REGISTERED) {
			if (processNodeNameSet (mac, buf, count, node)) {
				DEBUG_INFO ("Node name for node %d set to %s", node->getNodeId (), node->getNodeName ());
				if (notifyNewNode) {
					notifyNewNode (node->getMacAddress (), node->getNodeId (), node->getNodeName ());
				}
			} else {
				DEBUG_WARN ("Error setting node name for node %d", node->getNodeId ());
			}
		}
		break;
	default:
		DEBUG_WARN ("Received unknown EnigmaIOT message 0x%02X");
	}
}

bool EnigmaIOTGatewayClass::nodeNameSetRespose (Node* node, int8_t error) {
	/*
	 * ---------------------------------------------------
	 *| msgType (1) | IV (12) | Result code (1) | tag (16) |
	 * ---------------------------------------------------
	 */
	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		int8_t errorCode;
		uint8_t tag[TAG_LENGTH];
	} nodeNameSetResponse_msg;

	const unsigned int NNSRMSG_LEN = sizeof (nodeNameSetResponse_msg);

	nodeNameSetResponse_msg.msgType = NODE_NAME_RESULT;
	
	DEBUG_DBG ("Set node name Response. Error code: %d", error);

	CryptModule::random (nodeNameSetResponse_msg.iv, IV_LENGTH);

	DEBUG_VERBOSE ("IV: %s", printHexBuffer (nodeNameSetResponse_msg.iv, IV_LENGTH));
	
	nodeNameSetResponse_msg.errorCode = error;

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, (uint8_t*)&nodeNameSetResponse_msg, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from node key
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	if (!CryptModule::encryptBuffer ((uint8_t*)&(nodeNameSetResponse_msg.errorCode), sizeof (int8_t), // Encrypt error code only, 1 byte
									 nodeNameSetResponse_msg.iv, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), nodeNameSetResponse_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during encryption");
		return false;
	}

	DEBUG_VERBOSE ("Encrypted set node name response message: %s", printHexBuffer ((uint8_t*)&nodeNameSetResponse_msg, NNSRMSG_LEN));

	DEBUG_INFO (" -------> SEND SET NODE NAME RESPONSE");
	uint8_t* addr = node->getMacAddress ();
	char addrStr[ENIGMAIOT_ADDR_LEN * 3];
	if (comm->send (addr, (uint8_t*)&nodeNameSetResponse_msg, NNSRMSG_LEN) == 0) {
		DEBUG_INFO ("Set Node Name Response message sent to %s", mac2str(addr,addrStr));
		return true;
	} else {
		nodelist.unregisterNode (node);
		DEBUG_ERROR ("Error sending Set Node Name Response message to %s", mac2str (addr, addrStr));
		return false;
	}
}

bool EnigmaIOTGatewayClass::processNodeNameSet (const uint8_t mac[ENIGMAIOT_ADDR_LEN], uint8_t* buf, size_t count, Node* node) {
	/*
	* ----------------------------------------------------------------------
	*| msgType (1) | IV (12) | NodeID (2) | Node name (up to 32) | tag (16) |
	* ----------------------------------------------------------------------
	*/
	int8_t error = 0;

	char nodeName[NODE_NAME_LENGTH];
	memset ((void*)nodeName, 0, NODE_NAME_LENGTH);

	uint8_t iv_idx = 1;
	uint8_t nodeId_idx = iv_idx + IV_LENGTH;
	uint8_t nodeName_idx = nodeId_idx + sizeof (int16_t);
	uint8_t tag_idx = count - TAG_LENGTH;

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, buf, addDataLen); // Copy message upto iv
	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	uint8_t packetLen = count - TAG_LENGTH;

	if (!CryptModule::decryptBuffer (buf + nodeId_idx, packetLen - 1 - IV_LENGTH, // Decrypt from nodeId
									 buf + iv_idx, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), buf + tag_idx, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during decryption");
		error = -4; // Message error
	}

	if (!error) {
		DEBUG_VERBOSE ("Decripted node name set message: %s", printHexBuffer (buf, count - TAG_LENGTH));

		size_t nodeNameLen = tag_idx - nodeName_idx;

		DEBUG_DBG ("Node name length: %d bytes\n", nodeNameLen);

		if (nodeNameLen >= NODE_NAME_LENGTH) {
			nodeNameLen = NODE_NAME_LENGTH - 1;
		}

		memcpy ((void*)nodeName, (void*)(buf + nodeName_idx), nodeNameLen);

		error = nodelist.checkNodeName (nodeName, mac);
	}

	// TODO: Send response error
	nodeNameSetRespose (node, error);

	if (error) {
		return false;
	} else {
		node->setNodeName (nodeName);
		DEBUG_INFO ("Node name set to %s", node->getNodeName ());
		return true;
	}
}

bool EnigmaIOTGatewayClass::processControlMessage (const uint8_t mac[ENIGMAIOT_ADDR_LEN], uint8_t* buf, size_t count, Node* node) {
	/*
	* ----------------------------------------------------------------------------------------
	*| msgType (1) | IV (12) | length (2) | NodeId (2) | Counter (2) | Data (....) | Tag (16) |
	* ----------------------------------------------------------------------------------------
	*/

	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
	uint8_t data_idx = counter_idx + sizeof (int16_t);
	uint8_t tag_idx = count - TAG_LENGTH;

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, buf, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	uint8_t packetLen = count - TAG_LENGTH;

	if (!CryptModule::decryptBuffer (buf + length_idx, packetLen - 1 - IV_LENGTH, // Decrypt from nodeId
									 buf + iv_idx, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), buf + tag_idx, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during decryption");
		return false;
	}

	DEBUG_VERBOSE ("Decripted control message: %s", printHexBuffer (buf, count - TAG_LENGTH));

	// Check if command informs about a sleepy mode change
	const uint8_t* payload = buf + data_idx;
	if (payload[0] == control_message_type::SLEEP_ANS && (tag_idx - data_idx) >= 5) {
		uint32_t sleepTime;
		DEBUG_DBG ("Check if sleepy mode has changed for node");
		memcpy (&sleepTime, payload + 1, sizeof (uint32_t));
		if (sleepTime > 0) {
			DEBUG_DBG ("Set node to sleepy mode");
			node->setSleepy (true);
		} else {
			DEBUG_DBG ("Set node to non sleepy mode");
			node->setSleepy (false);
		}
	}

	DEBUG_DBG ("Payload length: %d bytes\n", tag_idx - data_idx);

	char* nodeName = node->getNodeName ();

	if (notifyData) {
		notifyData (const_cast<uint8_t*>(mac), buf + data_idx, tag_idx - data_idx, 0, true, ENIGMAIOT, nodeName ? nodeName : NULL);
	}

	return true;
}

bool EnigmaIOTGatewayClass::processUnencryptedDataMessage (const uint8_t mac[ENIGMAIOT_ADDR_LEN], uint8_t* buf, size_t count, Node* node) {
	/*
	* ------------------------------------------------------------------------
	*| msgType (1) | NodeId (2) | Counter (2) | PayloadType (1) | Data (....) |
	* ------------------------------------------------------------------------
	*/

	uint8_t nodeId_idx = 1;
	uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
	uint8_t payloadType_idx = counter_idx + sizeof (int16_t);
	uint8_t data_idx = payloadType_idx + sizeof (int8_t);

	uint16_t counter;
	size_t lostMessages = 0;

	uint8_t packetLen = count;

	DEBUG_VERBOSE ("Unencrypted data message: %s", printHexBuffer (buf, count));

	node->packetNumber++;

	memcpy (&counter, &buf[counter_idx], sizeof (uint16_t));
	if (useCounter) {
		if (counter > node->getLastMessageCounter ()) {
			lostMessages = counter - node->getLastMessageCounter () - 1;
			node->packetErrors += lostMessages;
			node->setLastMessageCounter (counter);
		} else {
			DEBUG_WARN ("Data counter error %d : %d", counter, node->getLastMessageCounter ());
			return false;
		}
	}

	char* nodeName = node->getNodeName ();

	if (notifyData) {
		notifyData (const_cast<uint8_t*>(mac), &(buf[data_idx]), count - data_idx, lostMessages, false, RAW, nodeName ? nodeName : NULL);
	}

	if (node->getSleepy ()) {
		if (node->qMessagePending) {
			DEBUG_INFO (" -------> DOWNLINK QUEUED DATA");
			flashTx = true;
			node->qMessagePending = false;
			return comm->send (node->getMacAddress (), node->queuedMessage, node->qMessageLength) == 0;
		}
	}

	return true;

}


bool EnigmaIOTGatewayClass::processDataMessage (const uint8_t mac[ENIGMAIOT_ADDR_LEN], uint8_t* buf, size_t count, Node* node, bool encrypted) {
	/*
	* ----------------------------------------------------------------------------------------
	*| msgType (1) | IV (12) | length (2) | NodeId (2) | Counter (2) | Data (....) | Tag (16) |
	* ----------------------------------------------------------------------------------------
	*/

	if (!encrypted) {
		return processUnencryptedDataMessage (mac, buf, count, node);
	}

	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
	uint8_t encoding_idx = counter_idx + sizeof (int16_t);
	uint8_t data_idx = encoding_idx + sizeof (int8_t);
	uint8_t tag_idx = count - TAG_LENGTH;

	uint16_t counter;
	size_t lostMessages = 0;

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, buf, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	uint8_t packetLen = count - TAG_LENGTH;

	if (!CryptModule::decryptBuffer (buf + length_idx, packetLen - 1 - IV_LENGTH, // Decrypt from nodeId
									 buf + iv_idx, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), buf + tag_idx, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during decryption");
		return false;
	}
	DEBUG_VERBOSE ("Decrypted data message: %s", printHexBuffer (buf, count - TAG_LENGTH));
	DEBUG_DBG ("Data payload encoding: 0x%02X", buf[encoding_idx]);
	node->packetNumber++;

	memcpy (&counter, &(buf[counter_idx]), sizeof (uint16_t));
	if (useCounter) {
		if (counter > node->getLastMessageCounter ()) {
			lostMessages = counter - node->getLastMessageCounter () - 1;
			node->packetErrors += lostMessages;
			node->setLastMessageCounter (counter);
		} else {
			return false;
		}
	}

	char* nodeName = node->getNodeName ();

	if (notifyData) {
		//DEBUG_WARN ("Notify data %d", input_queue->size());
		notifyData (const_cast<uint8_t*>(mac), &(buf[data_idx]), tag_idx - data_idx, lostMessages, false, (gatewayPayloadEncoding_t)(buf[encoding_idx]), nodeName ? nodeName : NULL);
	}

	if (node->getSleepy ()) {
		if (node->qMessagePending) {
			DEBUG_INFO (" -------> DOWNLINK QUEUED DATA");
			flashTx = true;
			node->qMessagePending = false;
			return comm->send (node->getMacAddress (), node->queuedMessage, node->qMessageLength) == 0;
		}
	}

	return true;

}

double EnigmaIOTGatewayClass::getPER (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	if (node->packetNumber > 0) {
		node->per = (double)node->packetErrors / (double)node->packetNumber;
	}

	return node->per;
}

uint32_t EnigmaIOTGatewayClass::getTotalPackets (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetNumber + getErrorPackets (address);
}

uint32_t EnigmaIOTGatewayClass::getErrorPackets (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetErrors;
}

double EnigmaIOTGatewayClass::getPacketsHour (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetsHour;
}


bool EnigmaIOTGatewayClass::downstreamDataMessage (Node* node, const uint8_t* data, size_t len, control_message_type_t controlData, gatewayPayloadEncoding_t encoding) {
	/*
	* ---------------------------------------------------------------------------
	*| msgType (1) | IV (12) | length (2) | NodeId (2)  | Data (....) | Tag (16) |
	* ---------------------------------------------------------------------------
	*/

	uint8_t buffer[MAX_MESSAGE_LENGTH];
	uint16_t packet_length;

	if (!node->isRegistered ()) {
		DEBUG_VERBOSE ("Error sending downstream. Node is not registered");
		return false;
	}

	uint16_t nodeId = node->getNodeId ();

	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t data_idx;
	uint8_t encoding_idx; // Only for user data
	if (controlData == USERDATA_GET || controlData == USERDATA_SET) {
		encoding_idx = nodeId_idx + sizeof (int16_t);
		data_idx = encoding_idx + sizeof (int8_t);
		buffer[encoding_idx] = encoding;
		packet_length = 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + 1 + len;
	} else {
		data_idx = nodeId_idx + sizeof (int16_t);
		packet_length = 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + len;
	}
	uint8_t tag_idx = data_idx + len;

	if (!data) {
		DEBUG_ERROR ("Downlink message buffer empty");
		return false;
	}
	if (len > MAX_MESSAGE_LENGTH - 25) {
		DEBUG_ERROR ("Downlink message too long: %d bytes", len);
		return false;
	}

	if (controlData == control_message_type::USERDATA_GET) {
		buffer[0] = (uint8_t)DOWNSTREAM_DATA_GET;
	} else if (controlData == control_message_type::USERDATA_SET) {
		buffer[0] = (uint8_t)DOWNSTREAM_DATA_SET;
	} else {
		buffer[0] = (uint8_t)DOWNSTREAM_CTRL_DATA;
	}

	CryptModule::random (buffer + iv_idx, IV_LENGTH);

	DEBUG_VERBOSE ("IV: %s", printHexBuffer (buffer + iv_idx, IV_LENGTH));

	memcpy (buffer + nodeId_idx, &nodeId, sizeof (uint16_t));

	//if (useCounter) {
	//    counter = node.getLastMessageCounter () + 1;
	//    node.setLastMessageCounter (counter);
	//    rtcmem_data.lastMessageCounter = counter;
	//}
	//else {
	//    counter = Crypto.random ();
	//}

	//memcpy (counter_p, &counter, sizeof (uint16_t));

	memcpy (buffer + data_idx, data, len);
	DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer + data_idx, len));

	memcpy (buffer + length_idx, &packet_length, sizeof (uint16_t));

	DEBUG_VERBOSE ("Downlink message: %s", printHexBuffer (buffer, packet_length));
	DEBUG_VERBOSE ("Message length: %d bytes", packet_length);

	//uint8_t* crypt_buf = buffer + length_idx;

	//size_t cryptLen = packet_length - length_idx;

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, buffer, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from Node Key
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	if (!CryptModule::encryptBuffer (buffer + length_idx, packet_length - addDataLen, // Encrypt from length
									 buffer + iv_idx, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of node key
									 aad, sizeof (aad), buffer + tag_idx, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during encryption");
		return false;
	}

	DEBUG_VERBOSE ("Encrypted downlink message: %s", printHexBuffer (buffer, packet_length + TAG_LENGTH));

	if (node->getSleepy ()) { // Queue message if node may be sleeping
		if (controlData != control_message_type::OTA) {
			DEBUG_VERBOSE ("Node is sleepy. Queing message");
			memcpy (node->queuedMessage, buffer, packet_length + TAG_LENGTH);
			//node->queuedMessage = buffer;
			node->qMessageLength = packet_length + TAG_LENGTH;
			node->qMessagePending = true;
			return true;
		} else {
			DEBUG_ERROR ("OTA is only possible with non sleepy nodes. Configure it accordingly first");
			return false;
		}
	} else {
		DEBUG_INFO (" -------> DOWNLINK DATA");
		flashTx = true;
		return comm->send (node->getMacAddress (), buffer, packet_length + TAG_LENGTH) == 0;
	}
}

bool  EnigmaIOTGatewayClass::invalidateKey (Node* node, gwInvalidateReason_t reason) {
	/*
	* --------------------------
	*| msgType (1) | reason (1) |
	* --------------------------
	*/

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t reason;
	} invalidateKey_msg;

#define IKMSG_LEN sizeof(invalidateKey_msg)

	invalidateKey_msg.msgType = INVALIDATE_KEY; // Server hello message

	invalidateKey_msg.reason = reason;

	DEBUG_VERBOSE ("Invalidate Key message: %s", printHexBuffer ((uint8_t*)&invalidateKey_msg, IKMSG_LEN));
	DEBUG_INFO (" -------> INVALIDATE_KEY");
	if (notifyNodeDisconnection) {
		uint8_t* mac = node->getMacAddress ();
		notifyNodeDisconnection (mac, reason);
	}
	return comm->send (node->getMacAddress (), (uint8_t*)&invalidateKey_msg, IKMSG_LEN) == 0;
}

bool EnigmaIOTGatewayClass::processClientHello (const uint8_t mac[ENIGMAIOT_ADDR_LEN], const uint8_t* buf, size_t count, Node* node) {
	/*
	* -------------------------------------------------------
	*| msgType (1) | random (12) | DH Kmaster (32) | Tag (16) |
	* -------------------------------------------------------
	*/

	bool sleepyNode;

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		uint8_t publicKey[KEY_LENGTH];
		uint32_t random;
		uint8_t tag[TAG_LENGTH];
	} clientHello_msg;

#define CHMSG_LEN sizeof(clientHello_msg)

	if (count < CHMSG_LEN) {
		DEBUG_WARN ("Message too short");
		return false;
	}

	memcpy (&clientHello_msg, buf, count);

	const uint8_t addDataLen = CHMSG_LEN - TAG_LENGTH - sizeof (uint32_t) - KEY_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, (uint8_t*)&clientHello_msg, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, gwConfig.networkKey + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	if (!CryptModule::decryptBuffer (clientHello_msg.publicKey, KEY_LENGTH + sizeof (uint32_t),
									 clientHello_msg.iv, IV_LENGTH,
									 gwConfig.networkKey, KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), clientHello_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during decryption");
		return false;
	}

	DEBUG_VERBOSE ("Decrypted Client Hello message: %s", printHexBuffer ((uint8_t*)&clientHello_msg, CHMSG_LEN - TAG_LENGTH));

	node->setEncryptionKey (clientHello_msg.publicKey);

	Crypto.getDH1 ();
	memcpy (myPublicKey, Crypto.getPubDHKey (), KEY_LENGTH);

	if (Crypto.getDH2 (node->getEncriptionKey ())) {
		CryptModule::getSHA256 (node->getEncriptionKey (), KEY_LENGTH);

		node->setKeyValid (true);
		node->setStatus (INIT);
		DEBUG_DBG ("Node key: %s", printHexBuffer (node->getEncriptionKey (), KEY_LENGTH));
	} else {
		nodelist.unregisterNode (node);
		char macstr[ENIGMAIOT_ADDR_LEN * 3];
		mac2str ((uint8_t*)mac, macstr);
		DEBUG_ERROR ("DH2 error with %s", macstr);
		return false;
	}

	sleepyNode = (clientHello_msg.random & 0x00000001U) == 1;
	node->setInitAsSleepy (sleepyNode);
	node->setSleepy (sleepyNode);

	DEBUG_VERBOSE ("This is a %s node", sleepyNode ? "sleepy" : "always awaken");

	return true;
}

bool EnigmaIOTGatewayClass::processClockRequest (const uint8_t mac[ENIGMAIOT_ADDR_LEN], const uint8_t* buf, size_t count, Node* node) {
	struct timeval tv;
	struct timezone tz;
	
	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		int64_t t1;
		uint8_t tag[TAG_LENGTH];
	} clockRequest_msg;

#define CRMSG_LEN sizeof(clockRequest_msg)

	if (count < CRMSG_LEN) {
		DEBUG_WARN ("Message too short");
		return false;
	}

	CryptModule::random (clockRequest_msg.iv, IV_LENGTH);

	DEBUG_VERBOSE ("IV: %s", printHexBuffer (clockRequest_msg.iv, IV_LENGTH));

	memcpy (&clockRequest_msg, buf, count);

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, buf, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	uint8_t packetLen = count - TAG_LENGTH;

	if (!CryptModule::decryptBuffer ((uint8_t*)&(clockRequest_msg.t1), sizeof (clock_t), // Decrypt from t2, 8 bytes
									 clockRequest_msg.iv, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), clockRequest_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during decryption");
		return false;
	}

	DEBUG_VERBOSE ("Decripted Clock Request message: %s", printHexBuffer ((uint8_t*)&clockRequest_msg, packetLen));

	node->t1 = clockRequest_msg.t1;

	// Get current time. If Gateway is synchronized to NTP server it sends real world time.
	gettimeofday (&tv, &tz);
	int64_t time_ms = tv.tv_sec;
	time_ms *= 1000;
	time_ms += tv.tv_usec / 1000;
	node->t2 = time_ms;

	DEBUG_DBG ("T1: %llu", node->t1);
	DEBUG_DBG ("T2: %llu", node->t2);
	DEBUG_VERBOSE ("Clock Request message: %s", printHexBuffer ((uint8_t*)&clockRequest_msg, CRMSG_LEN - TAG_LENGTH));

	return clockResponse (node);
}

bool EnigmaIOTGatewayClass::clockResponse (Node* node) {
	struct timeval tv;
	struct timezone tz;

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		int64_t t2;
		int64_t t3;
		uint8_t tag[TAG_LENGTH];
	} clockResponse_msg;

	const unsigned int CRSMSG_LEN = sizeof (clockResponse_msg);

	clockResponse_msg.msgType = CLOCK_RESPONSE;

	memcpy (&(clockResponse_msg.t2), &(node->t2), sizeof (int64_t));

	// Get current time. If Gateway is synchronized to NTP server it sends real world time.
	gettimeofday (&tv, &tz);
	int64_t time_ms = tv.tv_sec;
	time_ms *= 1000;
	time_ms += tv.tv_usec / 1000;
	node->t3 = time_ms;

	memcpy (&(clockResponse_msg.t3), &(node->t3), sizeof (int64_t));

	DEBUG_VERBOSE ("Clock Response message: %s", printHexBuffer ((uint8_t*)&clockResponse_msg, CRSMSG_LEN - TAG_LENGTH));

#ifdef DEBUG_ESP_PORT
	char mac[ENIGMAIOT_ADDR_LEN * 3];
	mac2str (node->getMacAddress (), mac);
#endif
	DEBUG_DBG ("T1: %llu", node->t1);
	DEBUG_DBG ("T2: %llu", node->t2);
	DEBUG_DBG ("T3: %llu", node->t3);

	const uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, (uint8_t*)&clockResponse_msg, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node->getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	if (!CryptModule::encryptBuffer ((uint8_t*)&(clockResponse_msg.t2), sizeof (int64_t) << 1, // Encrypt only from t2, 16 bytes
									 clockResponse_msg.iv, IV_LENGTH,
									 node->getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), clockResponse_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during encryption");
		return false;
	}

	DEBUG_VERBOSE ("Encrypted Clock Response message: %s", printHexBuffer ((uint8_t*)&clockResponse_msg, CRSMSG_LEN));

	DEBUG_INFO (" -------> CLOCK RESPONSE");
	if (comm->send (node->getMacAddress (), (uint8_t*)&clockResponse_msg, CRSMSG_LEN) == 0) {
		DEBUG_INFO ("Clock Response message sent to %s", mac);
		return true;
	} else {
		nodelist.unregisterNode (node);
		DEBUG_ERROR ("Error sending Clock Response message to %s", mac);
		return false;
	}
}

bool EnigmaIOTGatewayClass::serverHello (const uint8_t* key, Node* node) {
	/*
	* ------------------------------------------------------
	*| msgType (1) | random (12) | DH Kslave (32) | Tag (16) |
	* ------------------------------------------------------
	*/

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		uint8_t publicKey[KEY_LENGTH];
		uint16_t nodeId;
		uint32_t random;
		uint8_t tag[TAG_LENGTH];
	} serverHello_msg;

#define SHMSG_LEN sizeof(serverHello_msg)

	uint32_t random;

	if (!key) {
		DEBUG_ERROR ("NULL key");
		return false;
	}

	serverHello_msg.msgType = SERVER_HELLO; // Server hello message

	CryptModule::random (serverHello_msg.iv, IV_LENGTH);

	DEBUG_VERBOSE ("IV: %s", printHexBuffer (serverHello_msg.iv, IV_LENGTH));

	for (int i = 0; i < KEY_LENGTH; i++) {
		serverHello_msg.publicKey[i] = key[i];
	}

	uint16_t nodeId = node->getNodeId ();
	memcpy (&(serverHello_msg.nodeId), &nodeId, sizeof (uint16_t));

	random = Crypto.random ();
	memcpy (&(serverHello_msg.random), &random, RANDOM_LENGTH);

	DEBUG_VERBOSE ("Server Hello message: %s", printHexBuffer ((uint8_t*)&serverHello_msg, SHMSG_LEN - TAG_LENGTH));

	const uint8_t addDataLen = SHMSG_LEN - TAG_LENGTH - sizeof (uint32_t) - sizeof (uint16_t) - KEY_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, (uint8_t*)&serverHello_msg, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, gwConfig.networkKey + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	if (!CryptModule::encryptBuffer (serverHello_msg.publicKey, KEY_LENGTH + sizeof (uint16_t) + sizeof (uint32_t), // Encrypt from public key
									 serverHello_msg.iv, IV_LENGTH,
									 gwConfig.networkKey, KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), serverHello_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during encryption");
		return false;
	}

	DEBUG_VERBOSE ("Encrypted Server Hello message: %s", printHexBuffer ((uint8_t*)&serverHello_msg, SHMSG_LEN));

	flashTx = true;

#ifdef DEBUG_ESP_PORT
	char mac[ENIGMAIOT_ADDR_LEN * 3];
	mac2str (node->getMacAddress (), mac);
#endif
	DEBUG_INFO (" -------> SERVER_HELLO");
	if (comm->send (node->getMacAddress (), (uint8_t*)&serverHello_msg, SHMSG_LEN) == 0) {
		DEBUG_INFO ("Server Hello message sent to %s", mac);
		return true;
	} else {
		nodelist.unregisterNode (node);
		DEBUG_ERROR ("Error sending Server Hello message to %s", mac);
		return false;
	}
}

EnigmaIOTGatewayClass EnigmaIOTGateway;

