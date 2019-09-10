/**
  * @file EnigmaIOTGateway.cpp
  * @version 0.4.0
  * @date 10/09/2019
  * @author German Martin
  * @brief Library to build a gateway for EnigmaIoT system
  */

#include "EnigmaIOTGateway.h"
#include <FS.h>
#include "libb64/cdecode.h"
#include <Updater.h>

const char CONFIG_FILE[] = "/config.txt";

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

	for (size_t i = 0; i != str_size - target_size; ++i) {
		if (!memcmp (str + i, target, target_size)) {
			return str + i;
		}
	}

	return NULL;
}

//bool EnigmaIOTGatewayClass::processOTAMessage (uint8_t* msg, size_t msgLen, uint8_t* output) {
//	char* otaNumber = (char*)msg + 4;
//	uint8_t* otaData = (uint8_t*)memchr (msg, ',', msgLen) + 1;
//	size_t otaLen = msgLen - (otaData - msg);
//	size_t otaNumberLen = (uint8_t*)otaData - (uint8_t*)otaNumber - 1;
//	DEBUG_VERBOSE ("otaMsg = %p, otaNumber = %p, otaData = %p", msg, otaNumber, otaData);
//
//	char* number[6];
//	memset (number, 0, 6);
//	memcpy (number, otaNumber, otaNumberLen);
//
//	size_t decodedLen = base64_decode_chars ((char*)otaData, otaLen, (char*)output);
//
//	DEBUG_INFO ("OTA: %s otaLen %d, decodedLen %d", number, otaLen, decodedLen);
//
//	DEBUG_VERBOSE ("Decoded data: Len - %d -- %s", decodedLen, printHexBuffer (output, decodedLen));
//
//	return false;
//}

bool buildGetVersion (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	DEBUG_VERBOSE ("Build 'Get Version' message from: %s", printHexBuffer (inputData, inputLen));
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

int getNextNumber (char* &data, size_t &len/*, char* &position*/) {
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
	//if (tempLen >= 0) {
		number = atoi (strNum);
	//}
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
		c >= '0' && c <= '9' 
		|| c >= 'a' && c <= 'f'
		//|| c >= 'A' && c <= 'F'
		);
}

bool buildOtaMsg (uint8_t* data, size_t& dataLen, const uint8_t* inputData, size_t inputLen) {
	//char strNum[6];
	char* payload;
	size_t payloadLen;
	int strIndex;
	int number;
	uint8_t *tempData = data;

	DEBUG_VERBOSE ("Build 'OTA' message from: %s", inputData);

	payload = (char *)inputData;
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

	DEBUG_INFO ("OTA message number %u", msgIdx);
	//DEBUG_INFO ("Payload len = %u", payloadLen);
	//DEBUG_INFO ("Payload data: %s", payload);

	if (msgIdx > 0) {
		decodedLen += base64_decode_chars (payload, payloadLen, (char*)(data + 1 + sizeof (uint16_t)));
	} else {

		int8_t ASCIIHexToInt[] =
		{
			// ASCII
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
			-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

			// 0x80-FF (Omit this if you don't need to check for non-ASCII)
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
			-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		};

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

		DEBUG_DBG ("Number of OTA chunks %u", msgNum);
		DEBUG_DBG ("OTA length = %u bytes", fileSize);
		//DEBUG_INFO ("Payload data: %s", payload);

		//uint8_t* md5hex = tempData;// data + 1 + sizeof (uint16_t) + sizeof (uint16_t);

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

		//for (size_t i = 0; i < 32/*payloadLen*/; i += 2) {
		//	int8_t number;
		//	//DEBUG_VERBOSE ("Char1 %c", (char)payload[i]);
		//	number = ASCIIHexToInt[payload[i]];
		//	//DEBUG_VERBOSE ("Number1 %x", number);
		//	if (number < 0) {
		//		DEBUG_ERROR ("OTA message format error. MD5 string has no valid format");
		//		return false;
		//	}
		//	number <<= 4;
		//	//DEBUG_VERBOSE ("Char2 %c", (char)payload[i+1]);
		//	int8_t number2 = ASCIIHexToInt[payload[i + 1]];
		//	//DEBUG_VERBOSE ("Number2 %x", number2);
		//	if (number2 < 0) {
		//		DEBUG_ERROR ("OTA message format error. MD5 string has no valid format");
		//		return false;
		//	}
		//	number = number + number2;
		//	//DEBUG_VERBOSE ("Number %x", number);


		//	md5hex[i / 2] = number;
		//	decodedLen++;
		//	//***************************
		//}
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

	for (int i = 0; i < (inputLen - 1); i++) { // Check if all digits are number
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

bool EnigmaIOTGatewayClass::sendDownstream (uint8_t* mac, const uint8_t* data, size_t len, control_message_type_t controlData) {
	Node* node = nodelist.getNodeFromMAC (mac);
	uint8_t downstreamData[MAX_MESSAGE_LENGTH];

	if (len <= 0 && controlData == USERDATA)
		return false;

	DEBUG_VERBOSE ("Downstream: %s MessageType: %d", printHexBuffer (data, len), controlData);

	size_t dataLen = MAX_MESSAGE_LENGTH;

	bool result;

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
			DEBUG_ERROR ("Error building get Sleep message");
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
			DEBUG_ERROR ("Error building OTA message");
			return false;
		}
		DEBUG_VERBOSE ("OTA message. Len: %d Data %s", dataLen, printHexBuffer (downstreamData, dataLen));
		break;
	}

	DEBUG_INFO ("Send downstream");

	if (node) {
		if (controlData != control_message_type::USERDATA)
			return downstreamDataMessage (node, downstreamData, dataLen, controlData);
		else if (controlData == control_message_type::OTA) {
			if (node->getSleepy ()) {
				DEBUG_ERROR ("Node must be in non sleepy mode to receive OTA messages");
				return false;
			} else
				return downstreamDataMessage (node, data, len, controlData);
		} else
			return downstreamDataMessage (node, data, len, controlData);
	} else {
		char addr[18];
		DEBUG_ERROR ("Downlink destionation %s not found", mac2str (mac, addr));
		return false;
	}
}

bool EnigmaIOTGatewayClass::configWiFiManager () {
	AsyncWebServer server (80);
	DNSServer dns;
	char networkKey[33] = "";
	char channel[4];
	String (gwConfig.channel).toCharArray (channel, 4);

	AsyncWiFiManager wifiManager (&server, &dns);
	AsyncWiFiManagerParameter netKeyParam ("netkey", "NetworkKey", networkKey, 33, "required type=\"text\" maxlength=32");
	AsyncWiFiManagerParameter channelParam ("channel", "WiFi Channel", channel, 4, "required type=\"number\" min=\"0\" max=\"13\" step=\"1\"");

	wifiManager.addParameter (&netKeyParam);
	wifiManager.addParameter (&channelParam);
	wifiManager.setDebugOutput (true);
	wifiManager.setBreakAfterConfig (true);
	boolean result = wifiManager.startConfigPortal ("EnigmaIoTGateway");
	DEBUG_INFO ("==== Config Portal result ====");
	DEBUG_INFO ("Network Key: %s", netKeyParam.getValue ());
	DEBUG_INFO ("Channel: %s", channelParam.getValue ());
	if (result) {
		uint8_t keySize = netKeyParam.getValueLength ();
		if (keySize > KEY_LENGTH)
			keySize = KEY_LENGTH;
		memcpy (this->gwConfig.networkKey, netKeyParam.getValue (), keySize);
		CryptModule::getSHA256 (this->gwConfig.networkKey, KEY_LENGTH);
		gwConfig.channel = atoi (channelParam.getValue ());
		DEBUG_DBG ("Raw network Key: %s", printHexBuffer (this->gwConfig.networkKey, KEY_LENGTH));
		DEBUG_VERBOSE ("WiFi ESP-NOW channel: %d", gwConfig.networkKey);
	}

	return result;
}

bool EnigmaIOTGatewayClass::loadFlashData () {
	//SPIFFS.remove (CONFIG_FILE); // Only for testing

	if (SPIFFS.exists (CONFIG_FILE)) {
		DEBUG_DBG ("Opening %s file", CONFIG_FILE);
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
			DEBUG_DBG ("%s opened", CONFIG_FILE);
			size_t size = configFile.size ();
			if (size < sizeof (gateway_config_t)) {
				DEBUG_WARN ("Config file is corrupted. Deleting");
				SPIFFS.remove (CONFIG_FILE);
				return false;
			}
			configFile.read ((uint8_t*)(&gwConfig), sizeof (gateway_config_t));
			DEBUG_DBG ("Config file stored channel: %u", gwConfig.channel);
			configFile.close ();
			DEBUG_VERBOSE ("Gateway configuration successfuly read: %s", printHexBuffer ((uint8_t*)(&gwConfig), sizeof (gateway_config_t)));
			return true;
		}
	} else {
		DEBUG_WARN ("%s do not exist", CONFIG_FILE);
		return false;
	}

	return false;
}

bool EnigmaIOTGatewayClass::saveFlashData () {
	File configFile = SPIFFS.open (CONFIG_FILE, "w");
	if (!configFile) {
		DEBUG_WARN ("failed to open config file %s for writing", CONFIG_FILE);
		return false;
	}
	configFile.write ((uint8_t*)(&gwConfig), sizeof (gateway_config_t));
	configFile.close ();
	DEBUG_VERBOSE ("Gateway configuration saved to flash: %s", printHexBuffer ((uint8_t*)(&gwConfig), sizeof (gateway_config_t)));
	return true;
}

void EnigmaIOTGatewayClass::begin (Comms_halClass* comm, uint8_t* networkKey, bool useDataCounter) {
	this->comm = comm;
	this->useCounter = useDataCounter;

	if (networkKey) {
		memcpy (this->gwConfig.networkKey, networkKey, KEY_LENGTH);
		CryptModule::getSHA256 (this->gwConfig.networkKey, KEY_LENGTH);
	} else {
		if (!SPIFFS.begin ()) {
			DEBUG_ERROR ("Error mounting flash");
			return;
		}
		if (!loadFlashData ()) { // Load from flash
			if (configWiFiManager ()) {
				DEBUG_DBG ("Got configuration. Storing");
				if (saveFlashData ()) {
					DEBUG_DBG ("Network Key stored on flash");
				} else {
					DEBUG_ERROR ("Error saving data on flash");
				}
				ESP.restart ();
			} else {
				DEBUG_ERROR ("Configuration error. Restarting");
				ESP.restart ();
			}
		} else {
			DEBUG_INFO ("Configuration loaded from flash");
		}

		initWiFi (gwConfig.channel, COMM_GATEWAY);
		comm->begin (NULL, gwConfig.channel, COMM_GATEWAY);
		comm->onDataRcvd (rx_cb);
		comm->onDataSent (tx_cb);
	}
}

void EnigmaIOTGatewayClass::rx_cb (u8* mac_addr, u8* data, u8 len) {
	EnigmaIOTGateway.manageMessage (mac_addr, data, len);
}

void EnigmaIOTGatewayClass::tx_cb (u8* mac_addr, u8 status) {
	EnigmaIOTGateway.getStatus (mac_addr, status);
}

void EnigmaIOTGatewayClass::getStatus (u8* mac_addr, u8 status) {
	DEBUG_VERBOSE ("SENDStatus %s", status == 0 ? "OK" : "ERROR");
}

void EnigmaIOTGatewayClass::handle () {
	static unsigned long rxOntime;
	static unsigned long txOntime;

	if (flashRx) {
		rxOntime = millis ();
		digitalWrite (rxled, LOW);
		flashRx = false;
	}

	if (!digitalRead (rxled) && millis () - rxOntime > rxLedOnTime) {
		digitalWrite (rxled, HIGH);
	}

	if (flashTx) {
		txOntime = millis ();
		digitalWrite (txled, LOW);
		flashTx = false;
	}

	if (!digitalRead (txled) && millis () - txOntime > txLedOnTime) {
		digitalWrite (txled, HIGH);
	}

	// Clean up dead nodes
	for (int i = 0; i < NUM_NODES; i++) {
		Node* node = nodelist.getNodeFromID (i);
		if (node->isRegistered () && millis () - node->getLastMessageTime () > MAX_NODE_INACTIVITY) {
			// TODO. Trigger node expired event
			node->reset ();
		}
	}

}

void EnigmaIOTGatewayClass::manageMessage (const uint8_t* mac, const uint8_t* buf, uint8_t count) {
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
		if (espNowError == 0) {
			if (processClientHello (mac, buf, count, node)) {
				if (serverHello (myPublicKey, node)) {
					DEBUG_INFO ("Server Hello sent");
					node->setStatus (REGISTERED);
					node->setKeyValidFrom (millis ());
					node->setLastMessageCounter (0);
					node->setLastMessageTime ();
					if (notifyNewNode) {
						notifyNewNode (node->getMacAddress ());
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
				invalidateKey (node, WRONG_DATA);
				DEBUG_INFO ("Control message not OK");
			}
		} else {
			invalidateKey (node, UNREGISTERED_NODE);
		}
		break;
	case SENSOR_DATA:
		DEBUG_INFO (" <------- DATA");
		if (node->getStatus () == REGISTERED) {
			node->packetsHour = (double)1 / ((millis () - node->getLastMessageTime ()) / (double)3600000);
			if (processDataMessage (mac, buf, count, node)) {
				node->setLastMessageTime ();
				DEBUG_INFO ("Data OK");
				DEBUG_VERBOSE ("Key valid from %lu ms", millis () - node->getKeyValidFrom ());
				if (millis () - node->getKeyValidFrom () > MAX_KEY_VALIDITY) {
					invalidateKey (node, KEY_EXPIRED);
				}
			} else {
				invalidateKey (node, WRONG_DATA);
				DEBUG_INFO ("Data not OK");
			}

		} else {
			invalidateKey (node, UNREGISTERED_NODE);
		}
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
                DEBUG_INFO ("Clock request not OK");
            }

        } else {
            invalidateKey (node, UNREGISTERED_NODE);
        }
        break;

	}
}

bool EnigmaIOTGatewayClass::processControlMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
	/*
    * ----------------------------------------------------------------------------------------
    *| msgType (1) | IV (12) | length (2) | NodeId (2) | Counter (2) | Data (....) | Tag (16) |
    * ----------------------------------------------------------------------------------------
    */

	//uint8_t msgType_idx = 0;
	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
    uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
	uint8_t data_idx = counter_idx + sizeof (int16_t);
	uint8_t tag_idx = count - TAG_LENGTH;

    uint8_t addDataLen = 1 + IV_LENGTH;
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

	if (notifyData) {
		notifyData (mac, buf + data_idx, tag_idx - data_idx, 0, true);
	}

	return true;
}

bool EnigmaIOTGatewayClass::processDataMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
	/*
	* ----------------------------------------------------------------------------------------
	*| msgType (1) | IV (12) | length (2) | NodeId (2) | Counter (2) | Data (....) | Tag (16) |
	* ----------------------------------------------------------------------------------------
	*/

	//uint8_t msgType_idx = 0;
	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
	uint8_t data_idx = counter_idx + sizeof (int16_t);
	uint8_t tag_idx = count - TAG_LENGTH;

	uint16_t counter;
	size_t lostMessages = 0;

    uint8_t addDataLen = 1 + IV_LENGTH;
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

	node->packetNumber++;

	memcpy (&counter, &buf[counter_idx], sizeof (uint16_t));
	if (useCounter) {
		if (counter > node->getLastMessageCounter ()) {
			lostMessages = counter - node->getLastMessageCounter () - 1;
			node->packetErrors += lostMessages;
			node->setLastMessageCounter (counter);
		} else {
			return false;
		}
	}

	if (notifyData) {
		notifyData (mac, &buf[data_idx], tag_idx - data_idx, lostMessages, false);
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

	return node->packetNumber+getErrorPackets(address);
}

uint32_t EnigmaIOTGatewayClass::getErrorPackets (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetErrors;
}

double EnigmaIOTGatewayClass::getPacketsHour (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetsHour;
}


bool EnigmaIOTGatewayClass::downstreamDataMessage (Node* node, const uint8_t* data, size_t len, control_message_type_t controlData) {
	/*
	* ---------------------------------------------------------------------------
	*| msgType (1) | IV (12) | length (2) | NodeId (2)  | Data (....) | Tag (16) |
	* ---------------------------------------------------------------------------
	*/

	uint8_t buffer[MAX_MESSAGE_LENGTH];

	if (!node->isRegistered ()) {
		DEBUG_VERBOSE ("Error sending downstream. Node is not registered");
		return false;
	}

	uint16_t nodeId = node->getNodeId ();

	//uint8_t msgType_p = buffer;
	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t data_idx = nodeId_idx + sizeof (int16_t);
    uint8_t tag_idx = data_idx + len;

	if (!data) {
		DEBUG_ERROR ("Downlink message buffer empty");
		return false;
	}
	if (len > MAX_MESSAGE_LENGTH - 25) {
		DEBUG_ERROR ("Downlink message too long: %d bytes", len);
		return false;
	}

	if (controlData == control_message_type::USERDATA) {
        buffer[0] = (uint8_t)DOWNSTREAM_DATA;
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

	uint16_t packet_length = 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + len;

	memcpy (buffer + length_idx, &packet_length, sizeof (uint16_t));

	DEBUG_VERBOSE ("Downlink message: %s", printHexBuffer (buffer, packet_length));
	DEBUG_VERBOSE ("Message length: %d bytes", packet_length);

	uint8_t* crypt_buf = buffer + length_idx;

	size_t cryptLen = packet_length - length_idx;

    uint8_t addDataLen = 1 + IV_LENGTH;
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
		//uint32_t crc;
	} invalidateKey_msg;

#define IKMSG_LEN sizeof(invalidateKey_msg)

	invalidateKey_msg.msgType = INVALIDATE_KEY; // Server hello message

	invalidateKey_msg.reason = reason;

	DEBUG_VERBOSE ("Invalidate Key message: %s", printHexBuffer ((uint8_t*)& invalidateKey_msg, IKMSG_LEN));
	DEBUG_INFO (" -------> INVALIDATE_KEY");
	if (notifyNodeDisconnection) {
		uint8_t* mac = node->getMacAddress ();
		notifyNodeDisconnection (mac, reason);
	}
	return comm->send (node->getMacAddress (), (uint8_t*)& invalidateKey_msg, IKMSG_LEN) == 0;
}

bool EnigmaIOTGatewayClass::processClientHello (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
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

    uint8_t addDataLen = CHMSG_LEN - TAG_LENGTH - sizeof (uint32_t) - KEY_LENGTH;
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

	DEBUG_VERBOSE ("Decrypted Client Hello message: %s", printHexBuffer ((uint8_t*)& clientHello_msg, CHMSG_LEN - TAG_LENGTH));

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
		char macstr[18];
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

bool EnigmaIOTGatewayClass::processClockRequest (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        clock_t t1;
        uint8_t tag[TAG_LENGTH];
    } clockRequest_msg;

#define CRMSG_LEN sizeof(clockRequest_msg)

    if (count < CRMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

	memcpy (&clockRequest_msg, buf, count);

	node->t1 = clockRequest_msg.t1;
    node->t2 = millis();

	DEBUG_DBG ("T1: %u", node->t1);
	DEBUG_DBG ("T2: %u", node->t2);
	DEBUG_VERBOSE ("Clock Request message: %s", printHexBuffer ((uint8_t*)& clockRequest_msg, CRMSG_LEN - TAG_LENGTH));
	
    return clockResponse (node);
}

bool EnigmaIOTGatewayClass::clockResponse (Node* node) {

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        clock_t t2;
        clock_t t3;
        uint8_t tag[TAG_LENGTH];
    } clockResponse_msg;

#define CRSMSG_LEN sizeof(clockResponse_msg)

    clockResponse_msg.msgType = CLOCK_RESPONSE;

    //node->t2 = t2r;

    memcpy (&(clockResponse_msg.t2),&(node->t2),sizeof(clock_t));

    node->t3 = millis();

    memcpy (&(clockResponse_msg.t3), &(node->t3), sizeof (clock_t));

	DEBUG_VERBOSE ("Clock Response message: %s", printHexBuffer ((uint8_t*)& clockResponse_msg, CRSMSG_LEN - TAG_LENGTH));

#ifdef DEBUG_ESP_PORT
    char mac[18];
    mac2str (node->getMacAddress (), mac);
#endif
	DEBUG_DBG ("T1: %u", node->t1);
	DEBUG_DBG ("T2: %u", node->t2);
	DEBUG_DBG ("T3: %u", node->t3);

	DEBUG_INFO (" -------> CLOCK RESPONSE");
    if (comm->send (node->getMacAddress (), (uint8_t*)& clockResponse_msg, CRSMSG_LEN) == 0) {
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

	DEBUG_VERBOSE ("Server Hello message: %s", printHexBuffer ((uint8_t*)& serverHello_msg, SHMSG_LEN - TAG_LENGTH));

    uint8_t addDataLen = SHMSG_LEN - TAG_LENGTH - sizeof (uint32_t) - sizeof (uint16_t) - KEY_LENGTH;
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

	DEBUG_VERBOSE ("Encrypted Server Hello message: %s", printHexBuffer ((uint8_t*)& serverHello_msg, SHMSG_LEN));

	flashTx = true;

#ifdef DEBUG_ESP_PORT
	char mac[18];
	mac2str (node->getMacAddress (), mac);
#endif
	DEBUG_INFO (" -------> SERVER_HELLO");
	if (comm->send (node->getMacAddress (), (uint8_t*)& serverHello_msg, SHMSG_LEN) == 0) {
		DEBUG_INFO ("Server Hello message sent to %s", mac);
		return true;
	} else {
		nodelist.unregisterNode (node);
		DEBUG_ERROR ("Error sending Server Hello message to %s", mac);
		return false;
	}
}

bool EnigmaIOTGatewayClass::checkCRC (const uint8_t* buf, size_t count, const uint32_t* crc) {
	uint32_t recvdCRC;

	memcpy (&recvdCRC, crc, sizeof (uint32_t)); // Use of memcpy is a must to ensure code does not try to read non memory aligned int
	uint32_t _crc = CRC32::calculate (buf, count);
	DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X %s", _crc, recvdCRC, (_crc == recvdCRC) ? "OK" : "FAIL");
	return (_crc == recvdCRC);
}


EnigmaIOTGatewayClass EnigmaIOTGateway;

