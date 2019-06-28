/**
  * @file EnigmaIOTGateway.cpp
  * @version 0.2.0
  * @date 28/06/2019
  * @author German Martin
  * @brief Library to build a gateway for EnigmaIoT system
  */

#include "EnigmaIOTGateway.h"
#include <FS.h>

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

bool EnigmaIOTGatewayClass::sendDownstream (uint8_t* mac, const uint8_t* data, size_t len) {
	Node* node = nodelist.getNodeFromMAC (mac);

	if (node) {
		return downstreamDataMessage (node, data, len);
	}
	else {
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
	DEBUG_DBG ("==== Config Portal result ====");
	DEBUG_DBG ("Network Key: %s", netKeyParam.getValue ());
	DEBUG_DBG ("Channel: %s", channelParam.getValue ());
	if (result) {
		uint8_t keySize = netKeyParam.getValueLength ();
		if (netKeyParam.getValueLength () > KEY_LENGTH)
			keySize = KEY_LENGTH;
		memcpy (this->gwConfig.networkKey, netKeyParam.getValue (), keySize);
		gwConfig.channel = atoi (channelParam.getValue ());
		DEBUG_VERBOSE ("Raw network Key: %s", printHexBuffer(this->gwConfig.networkKey,KEY_LENGTH));
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
			configFile.close ();
			DEBUG_VERBOSE ("Gateway configuration successfuly read: %s", printHexBuffer ((uint8_t*)(&gwConfig), sizeof (gateway_config_t)));
			return true;
		}
	}
	else {
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
	configFile.write ((uint8_t*)(&gwConfig), sizeof(gateway_config_t));
	configFile.close ();
	DEBUG_VERBOSE ("Gateway configuration saved to flash: %s", printHexBuffer ((uint8_t*)(&gwConfig), sizeof (gateway_config_t)));
	return true; 
}

void EnigmaIOTGatewayClass::begin (Comms_halClass* comm, uint8_t* networkKey, bool useDataCounter) {
	this->comm = comm;
	this->useCounter = useDataCounter;

	if (networkKey) {
		memcpy (this->gwConfig.networkKey, networkKey, KEY_LENGTH);
	}
	else {
		if (!SPIFFS.begin ()) {
			DEBUG_ERROR ("Error mounting flash");
			return;
		}
		if (!loadFlashData ()) { // Load from flash
			if (configWiFiManager ()) {
				DEBUG_DBG ("Got configuration. Storing");
				if (saveFlashData ()) {
					DEBUG_DBG ("Network Key stored on flash");
				}
				else {
					DEBUG_ERROR ("Error saving data on flash");
				}
				ESP.restart ();
			}
			else {
				DEBUG_ERROR ("Configuration error. Restarting");
				ESP.restart ();
			}
		}

		//initWiFi ();
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

	//node->packetNumber++;

	int espNowError = 0; // May I remove this??

	switch (buf[0]) {
	case CLIENT_HELLO:
		// TODO: Do no accept new Client Hello if registration is on process on any node??
		// May cause undesired behaviour in case a node registration message is lost
		DEBUG_INFO (" <------- CLIENT HELLO");
		if (espNowError == 0) {
			if (processClientHello (mac, buf, count, node)) {
				node->setStatus (WAIT_FOR_KEY_EXCH_FINISHED);
				delay (1000);
				if (serverHello (myPublicKey, node)) {
					DEBUG_INFO ("Server Hello sent");
				}
				else {
					node->reset ();
					DEBUG_INFO ("Error sending Server Hello");
				}

			}
			else {
				// Ignore message in case of error
				//invalidateKey (node, WRONG_CLIENT_HELLO);
				node->reset ();
				DEBUG_ERROR ("Error processing client hello");
			}
		}
		else {
			DEBUG_ERROR ("Error adding peer %d", espNowError);
		}
		break;
	case KEY_EXCHANGE_FINISHED:
		if (node->getStatus () == WAIT_FOR_KEY_EXCH_FINISHED) {
			DEBUG_INFO (" <------- KEY EXCHANGE FINISHED");
			if (espNowError == 0) {
				if (processKeyExchangeFinished (mac, buf, count, node)) {
					if (cipherFinished (node)) {
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
					}
					else {
						node->reset ();
					}
				}
				else {
					invalidateKey (node, WRONG_EXCHANGE_FINISHED);
					node->reset ();
				}
			}
		}
		else {
			DEBUG_INFO (" <------- unsolicited KEY EXCHANGE FINISHED");
		}
		break;
	case CONTROL_DATA:
		DEBUG_INFO (" <------- CONTROL MESSAGE");
		if (node->getStatus () == REGISTERED) {
			if (processControlMessage (mac, buf, count, node)) {
				DEBUG_INFO ("Control message OK");
			}
			else {
				invalidateKey (node, WRONG_DATA);
				DEBUG_INFO ("Data not OK");
			}
		}
		else {
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
			}
			else {
				invalidateKey (node, WRONG_DATA);
				DEBUG_INFO ("Data not OK");
			}

		}
		else {
			invalidateKey (node, UNREGISTERED_NODE);
		}
	}
}

bool EnigmaIOTGatewayClass::processControlMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
	/*
	* ---------------------------------------------------------------------------------------
	*| msgType (1) | IV (16) | length (2) | NodeId (2) | Counter (2) | Data (....) | CRC (4) |
	* ---------------------------------------------------------------------------------------
	*/

	//uint8_t msgType_idx = 0;
	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t counter_idx = nodeId_idx + sizeof (int16_t); // Counter is always 0
	uint8_t data_idx = counter_idx + sizeof (int16_t);
	uint8_t crc_idx = count - CRC_LENGTH;

	//uint8_t *iv;
	uint32_t crc32;
	//uint16_t counter;
	//size_t lostMessages = 0;

	Crypto.decryptBuffer (
		const_cast<uint8_t*>(&buf[length_idx]),
		const_cast<uint8_t*>(&buf[length_idx]),
		count - length_idx,
		const_cast<uint8_t*>(&buf[iv_idx]),
		IV_LENGTH,
		node->getEncriptionKey (),
		KEY_LENGTH
	);
	DEBUG_VERBOSE ("Decripted control message: %s", printHexBuffer (buf, count));

	memcpy (&crc32, &buf[crc_idx], CRC_LENGTH);
	if (!checkCRC (buf, count - 4, &crc32)) {
		DEBUG_WARN ("Wrong CRC");
		node->packetErrors++;
		return false;
	}

	//TODO Send message to Serial

	DEBUG_DBG ("Payload length: %d bytes\n", crc_idx - data_idx);
	char macstr[18];
	mac2str (mac, macstr);
	Serial.printf ("~/%s/control/version;", macstr);
	Serial.write ((uint8_t*) & (buf[data_idx]), crc_idx - data_idx);
	Serial.println ();

	return true;
}

bool EnigmaIOTGatewayClass::processDataMessage (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
	/*
	* ---------------------------------------------------------------------------------------
	*| msgType (1) | IV (16) | length (2) | NodeId (2) | Counter (2) | Data (....) | CRC (4) |
	* ---------------------------------------------------------------------------------------
	*/

	//uint8_t msgType_idx = 0;
	uint8_t iv_idx = 1;
	uint8_t length_idx = iv_idx + IV_LENGTH;
	uint8_t nodeId_idx = length_idx + sizeof (int16_t);
	uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
	uint8_t data_idx = counter_idx + sizeof (int16_t);
	uint8_t crc_idx = count - CRC_LENGTH;

	//uint8_t *iv;
	uint32_t crc32;
	uint16_t counter;
	size_t lostMessages = 0;

	Crypto.decryptBuffer (
		const_cast<uint8_t*>(&buf[length_idx]),
		const_cast<uint8_t*>(&buf[length_idx]),
		count - length_idx,
		const_cast<uint8_t*>(&buf[iv_idx]),
		IV_LENGTH,
		node->getEncriptionKey (),
		KEY_LENGTH
	);
	DEBUG_VERBOSE ("Decripted data message: %s", printHexBuffer (buf, count));

	node->packetNumber++;

	memcpy (&counter, &buf[counter_idx], sizeof (uint16_t));
	if (useCounter) {
		if (counter > node->getLastMessageCounter ()) {
			lostMessages = counter - node->getLastMessageCounter () - 1;
			node->packetErrors += lostMessages;
			node->setLastMessageCounter (counter);
		}
		else {
			return false;
		}
	}

	memcpy (&crc32, &buf[crc_idx], CRC_LENGTH);

	if (!checkCRC (buf, count - 4, &crc32)) {
		DEBUG_WARN ("Wrong CRC");
		node->packetErrors++;
		return false;
	}

	if (notifyData) {
		notifyData (mac, &buf[data_idx], crc_idx - data_idx, lostMessages);
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

	return node->packetNumber;
}

uint32_t EnigmaIOTGatewayClass::getErrorPackets (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetErrors;
}

double EnigmaIOTGatewayClass::getPacketsHour (uint8_t* address) {
	Node* node = nodelist.getNewNode (address);

	return node->packetsHour;
}


bool EnigmaIOTGatewayClass::downstreamDataMessage (Node* node, const uint8_t* data, size_t len) {
	/*
	* --------------------------------------------------------------------------
	*| msgType (1) | IV (16) | length (2) | NodeId (2)  | Data (....) | CRC (4) |
	* --------------------------------------------------------------------------
	*/

	uint8_t buffer[MAX_MESSAGE_LENGTH];
	uint32_t crc32;

	if (!node->isRegistered ()) {
		DEBUG_VERBOSE ("Error sending downstream. Node is not registered");
		return false;
	}

	uint16_t nodeId = node->getNodeId ();

	uint8_t* msgType_p = buffer;
	uint8_t* iv_p = buffer + 1;
	uint8_t* length_p = buffer + 1 + IV_LENGTH;
	uint8_t* nodeId_p = buffer + 1 + IV_LENGTH + sizeof (int16_t);
	uint8_t* data_p = buffer + 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t);

	if (!data) {
		DEBUG_ERROR ("Downlink message buffer empty");
		return false;
	}
	if (len > MAX_MESSAGE_LENGTH - 25) {
		DEBUG_ERROR ("Downlink message too long: %d bytes", len);
		return false;
	}

	*msgType_p = (uint8_t)DOWNSTREAM_DATA;

	CryptModule::random (iv_p, IV_LENGTH);

	DEBUG_VERBOSE ("IV: %s", printHexBuffer (iv_p, IV_LENGTH));

	memcpy (nodeId_p, &nodeId, sizeof (uint16_t));

	//if (useCounter) {
	//    counter = node.getLastMessageCounter () + 1;
	//    node.setLastMessageCounter (counter);
	//    rtcmem_data.lastMessageCounter = counter;
	//}
	//else {
	//    counter = Crypto.random ();
	//}

	//memcpy (counter_p, &counter, sizeof (uint16_t));

	memcpy (data_p, data, len);

	uint16_t packet_length = 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + len;

	memcpy (length_p, &packet_length, sizeof (uint16_t));

	crc32 = CRC32::calculate (buffer, packet_length);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	uint8_t* crc_p = (uint8_t*)(buffer + packet_length);

	memcpy (crc_p, &crc32, CRC_LENGTH);

	DEBUG_VERBOSE ("Downlink message: %s", printHexBuffer (buffer, packet_length + CRC_LENGTH));
	DEBUG_VERBOSE ("Message length: %d bytes", packet_length + CRC_LENGTH);

	uint8_t* crypt_buf = length_p; // buffer + 1 + IV_LENGTH;

	size_t cryptLen = packet_length + CRC_LENGTH - 1 - IV_LENGTH;

	Crypto.encryptBuffer (crypt_buf, crypt_buf, cryptLen, iv_p, IV_LENGTH, node->getEncriptionKey (), KEY_LENGTH);

	DEBUG_VERBOSE ("Encrypted downlink message: %s", printHexBuffer (buffer, packet_length + CRC_LENGTH));

	if (node->getSleepy ()) { // Queue message if node may be sleeping
		DEBUG_VERBOSE ("Node is sleepy. Queing message");
		memcpy (node->queuedMessage, buffer, packet_length + CRC_LENGTH);
		//node->queuedMessage = buffer;
		node->qMessageLength = packet_length + CRC_LENGTH;
		node->qMessagePending = true;
		return true;
	}
	else {
		DEBUG_INFO (" -------> DOWNLINK DATA");
		flashTx = true;
		return comm->send (node->getMacAddress (), buffer, packet_length + CRC_LENGTH) == 0;
	}
}


bool EnigmaIOTGatewayClass::processKeyExchangeFinished (const uint8_t mac[6], const uint8_t* buf, size_t count, Node* node) {
	/*
	* -------------------------------------------------------------------------
	*| msgType (1) | IV (16) | random (31 bits) | SleepyNode (1 bit) | CRC (4) |
	* -------------------------------------------------------------------------
	*/

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		uint32_t random;
		uint32_t crc;
	} keyExchangeFinished_msg;

#define KEFMSG_LEN sizeof(keyExchangeFinished_msg)

	uint32_t crc32;
	bool sleepyNode;

	if (count < KEFMSG_LEN) {
		DEBUG_WARN ("Wrong message");
		return false;
	}

	memcpy (&keyExchangeFinished_msg, buf, KEFMSG_LEN);

	CryptModule::networkDecrypt (keyExchangeFinished_msg.iv, 1, gwConfig.networkKey, KEY_LENGTH);

	DEBUG_VERBOSE ("Netowrk decrypted Key Exchange Finished message: %s", printHexBuffer ((uint8_t*)& keyExchangeFinished_msg, KEFMSG_LEN));

	Crypto.decryptBuffer (
		(uint8_t*) & (keyExchangeFinished_msg.random),
		(uint8_t*) & (keyExchangeFinished_msg.random),
		RANDOM_LENGTH + CRC_LENGTH,
		keyExchangeFinished_msg.iv,
		IV_LENGTH,
		node->getEncriptionKey (),
		KEY_LENGTH
	);

	DEBUG_VERBOSE ("Decripted Key Exchange Finished message: %s", printHexBuffer ((uint8_t*)& keyExchangeFinished_msg, KEFMSG_LEN));

	memcpy (&crc32, &(keyExchangeFinished_msg.crc), CRC_LENGTH);

	if (!checkCRC ((uint8_t*)& keyExchangeFinished_msg, KEFMSG_LEN - CRC_LENGTH, &crc32)) {
		DEBUG_WARN ("Wrong CRC");
		return false;
	}

	sleepyNode = (keyExchangeFinished_msg.random & 0x00000001U) == 1;
	node->setSleepy (sleepyNode);

	DEBUG_VERBOSE ("This is a %s node", sleepyNode ? "sleepy" : "always awaken");

	return true;
}

bool EnigmaIOTGatewayClass::cipherFinished (Node* node) {
	/*
	* -----------------------------------------------------------
	*| msgType (1) | IV (16) | nodeId (2) | random (4) | CRC (4) |
	* -----------------------------------------------------------
	*/

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		uint16_t nodeId;
		uint32_t random;
		uint32_t crc;
	} cipherFinished_msg;

#define CFMSG_LEN sizeof(cipherFinished_msg)

	uint32_t crc32;
	uint32_t random;

	cipherFinished_msg.msgType = CYPHER_FINISHED; // Server hello message

	CryptModule::random (cipherFinished_msg.iv, IV_LENGTH);
	DEBUG_VERBOSE ("IV: %s", printHexBuffer (cipherFinished_msg.iv, IV_LENGTH));

	uint16_t nodeId = node->getNodeId ();
	memcpy (&(cipherFinished_msg.nodeId), &nodeId, sizeof (uint16_t));

	random = Crypto.random ();

	memcpy (&(cipherFinished_msg.random), &random, RANDOM_LENGTH);

	crc32 = CRC32::calculate ((uint8_t*)& cipherFinished_msg, CFMSG_LEN - CRC_LENGTH);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	memcpy (&(cipherFinished_msg.crc), (uint8_t*)& crc32, CRC_LENGTH);

	DEBUG_VERBOSE ("Cipher Finished message: %s", printHexBuffer ((uint8_t*)& cipherFinished_msg, CFMSG_LEN));

	Crypto.encryptBuffer (
		(uint8_t*) & (cipherFinished_msg.nodeId),
		(uint8_t*) & (cipherFinished_msg.nodeId),
		2 + RANDOM_LENGTH + CRC_LENGTH,
		cipherFinished_msg.iv,
		IV_LENGTH,
		node->getEncriptionKey (),
		KEY_LENGTH
	);

	DEBUG_VERBOSE ("Encripted Cipher Finished message: %s", printHexBuffer ((uint8_t*)& cipherFinished_msg, CFMSG_LEN));

	CryptModule::networkEncrypt (cipherFinished_msg.iv, 1, gwConfig.networkKey, KEY_LENGTH);

	DEBUG_VERBOSE ("Network encrypted Key Exchange Finished message: %s", printHexBuffer ((uint8_t*)& cipherFinished_msg, CFMSG_LEN));

	flashTx = true;
	DEBUG_INFO (" -------> CYPHER_FINISHED");
	return comm->send (node->getMacAddress (), (uint8_t*)& cipherFinished_msg, CFMSG_LEN) == 0;
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

	//uint32_t crc = CRC32::calculate (buffer, 2);
	//DEBUG_VERBOSE ("CRC32 = 0x%08X", crc);
	//memcpy (&invalidateKey_msg.crc, (uint8_t *)&crc, CRC_LENGTH);
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
	*| msgType (1) | random (16) | DH Kmaster (32) | CRC (4) |
	* -------------------------------------------------------
	*/

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		uint8_t publicKey[KEY_LENGTH];
		uint32_t crc;
	} clientHello_msg;

#define CHMSG_LEN sizeof(clientHello_msg)

	uint32_t crc32;

	if (count < CHMSG_LEN) {
		DEBUG_WARN ("Message too short");
		return false;
	}

	memcpy (&clientHello_msg, buf, count);

	CryptModule::networkDecrypt (clientHello_msg.iv, 3, gwConfig.networkKey, KEY_LENGTH);

	DEBUG_VERBOSE ("Netowrk decrypted Client Hello message: %s", printHexBuffer ((uint8_t*)& clientHello_msg, CHMSG_LEN));

	memcpy (&crc32, &(clientHello_msg.crc), sizeof (uint32_t));

	if (!checkCRC ((uint8_t*)& clientHello_msg, CHMSG_LEN - CRC_LENGTH, &crc32)) {
		DEBUG_WARN ("Wrong CRC");
		return false;
	}

	node->setEncryptionKey (clientHello_msg.publicKey);

	Crypto.getDH1 ();
	memcpy (myPublicKey, Crypto.getPubDHKey (), KEY_LENGTH);

	if (Crypto.getDH2 (node->getEncriptionKey ())) {
		node->setKeyValid (true);
		node->setStatus (INIT);
		DEBUG_INFO ("Node key: %s", printHexBuffer (node->getEncriptionKey (), KEY_LENGTH));
	}
	else {
		nodelist.unregisterNode (node);
		char macstr[18];
		mac2str ((uint8_t*)mac, macstr);
		DEBUG_ERROR ("DH2 error with %s", macstr);
		return false;
	}
	return true;
}


bool EnigmaIOTGatewayClass::serverHello (const uint8_t* key, Node* node) {
	/*
	* ------------------------------------------------------
	*| msgType (1) | random (16) | DH Kslave (32) | CRC (4) |
	* ------------------------------------------------------
	*/

	struct __attribute__ ((packed, aligned (1))) {
		uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		uint8_t publicKey[KEY_LENGTH];
		uint32_t crc;
	} serverHello_msg;

#define SHMSG_LEN sizeof(serverHello_msg)

	uint32_t crc32;

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

	crc32 = CRC32::calculate ((uint8_t*)& serverHello_msg, SHMSG_LEN - CRC_LENGTH);
	DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

	memcpy (&(serverHello_msg.crc), &crc32, CRC_LENGTH);

	DEBUG_VERBOSE ("Server Hello message: %s", printHexBuffer ((uint8_t*)& serverHello_msg, SHMSG_LEN));

	CryptModule::networkEncrypt (serverHello_msg.iv, 3, gwConfig.networkKey, KEY_LENGTH);

	DEBUG_VERBOSE ("Network encrypted Server Hello message: %s", printHexBuffer ((uint8_t*)& serverHello_msg, SHMSG_LEN));

	flashTx = true;

#ifdef DEBUG_ESP_PORT
	char mac[18];
	mac2str (node->getMacAddress (), mac);
#endif
	DEBUG_INFO (" -------> SERVER_HELLO");
	if (comm->send (node->getMacAddress (), (uint8_t*)& serverHello_msg, SHMSG_LEN) == 0) {
		DEBUG_INFO ("Server Hello message sent to %s", mac);
		return true;
	}
	else {
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

