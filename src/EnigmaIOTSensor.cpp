/**
  * @file EnigmaIOTSensor.cpp
  * @version 0.2.0
  * @date 28/06/2019
  * @author German Martin
  * @brief Library to build a node for EnigmaIoT system
  */

#include "EnigmaIOTSensor.h"
#include <FS.h>
#include <MD5Builder.h>
#include <Updater.h>
#include <StreamString.h>

const char CONFIG_FILE[] = "/config.txt";

void EnigmaIOTSensorClass::setLed (uint8_t led, time_t onTime) {
    this->led = led;
    ledOnTime = onTime;
}

void clearRtcData (rtcmem_data_t* data) {
	memset (data->nodeKey, 0, KEY_LENGTH);
	data->lastMessageCounter = 0;
	data->nodeId = 0;
	data->channel = 3;
	memset (data->gateway, 0, 6);
	memset (data->networkKey, 0, KEY_LENGTH);
	data->nodeRegisterStatus = UNREGISTERED;
	data->sleepy = false;
	data->nodeKeyValid = false;
}

void dumpRtcData (rtcmem_data_t* data, uint8_t *gateway = NULL) {
	Serial.println ("RTC MEM DATA:");
	if (data) {
		Serial.printf (" -- CRC: %s\n", printHexBuffer ((uint8_t*)&(data->crc32), sizeof (uint32_t)));
		Serial.printf (" -- Node Key: %s\n", printHexBuffer (data->nodeKey, KEY_LENGTH));
		Serial.printf (" -- Node key is %svalid\n", data->nodeKeyValid ? "":"NOT ");
		Serial.printf (" -- Node status is %s\n", data->nodeRegisterStatus == REGISTERED ? "REGISTERED" :"NOT REGISTERED");
		Serial.printf (" -- Last message counter: %d\n", data->lastMessageCounter);
		Serial.printf (" -- NodeID: %d\n", data->nodeId);
		Serial.printf (" -- Channel: %d\n", data->channel);
		char gwAddress[18];
		Serial.printf (" -- Gateway: %s\n", mac2str(data->gateway,gwAddress));
		if (gateway)
			Serial.printf (" -- Gateway address: %s\n", mac2str (gateway, gwAddress));
		Serial.printf (" -- Network Key: %s\n", printHexBuffer (data->networkKey, KEY_LENGTH));
		Serial.printf (" -- Mode: %s\n", data->sleepy?"sleepy":"non sleepy");
	}
	else {
		Serial.println ("rtcmem_data pointer is NULL");
	}
}

bool EnigmaIOTSensorClass::loadRTCData () {
	if (ESP.rtcUserMemoryRead (0, (uint32_t*)& rtcmem_data, sizeof (rtcmem_data))) {
		DEBUG_VERBOSE ("Read RTCData: %s", printHexBuffer ((uint8_t*)& rtcmem_data, sizeof (rtcmem_data)));
	} else {
		DEBUG_ERROR ("Error reading RTC memory");
		clearRtcData (&rtcmem_data);
		return false;
	}
	if (!checkCRC ((uint8_t*)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t), &rtcmem_data.crc32)) {
		DEBUG_DBG ("RTC Data is not valid");
		clearRtcData (&rtcmem_data);
		return false;
	} else {
		node.setEncryptionKey (rtcmem_data.nodeKey);
		node.setKeyValid (rtcmem_data.nodeKeyValid);
		if (rtcmem_data.nodeKeyValid)
		    node.setKeyValidFrom (millis ());
		node.setLastMessageCounter (rtcmem_data.lastMessageCounter);
		node.setLastMessageTime ();
		node.setNodeId (rtcmem_data.nodeId);
		// setChannel (rtcmem_data.channel);
		//channel = rtcmem_data.channel;
		//memcpy (gateway, rtcmem_data.gateway, comm->getAddressLength ()); // setGateway
		//memcpy (networkKey, rtcmem_data.networkKey, KEY_LENGTH);
		node.setSleepy (rtcmem_data.sleepy);
		// set default sleep time if it was not set
		if (rtcmem_data.sleepy && rtcmem_data.sleepTime == 0) {
			rtcmem_data.sleepTime = DEFAULT_SLEEP_TIME;
		}
		node.setStatus (rtcmem_data.nodeRegisterStatus);
		DEBUG_DBG ("Set %s mode", node.getSleepy () ? "sleepy" : "non sleepy");
#if DEBUG_LEVEL >= VERBOSE
		dumpRtcData (&rtcmem_data/*,gateway*/);
#endif

	}
	return true;

}

bool EnigmaIOTSensorClass::loadFlashData () {
	//SPIFFS.remove (CONFIG_FILE); // Only for testing

	if (SPIFFS.exists (CONFIG_FILE)) {
		DEBUG_DBG ("Opening %s file", CONFIG_FILE);
		File configFile = SPIFFS.open (CONFIG_FILE, "r");
		if (configFile) {
			DEBUG_DBG ("%s opened", CONFIG_FILE);
			size_t size = configFile.size ();
			if (size < sizeof(rtcmem_data)) {
				DEBUG_WARN ("Config file is corrupted. Deleting");
				SPIFFS.remove (CONFIG_FILE);
				return false;
			}
			configFile.read ((uint8_t*)(&rtcmem_data), sizeof(rtcmem_data));
			configFile.close ();
			DEBUG_VERBOSE ("Configuration successfuly read: %s", printHexBuffer ((uint8_t*)&rtcmem_data, sizeof(rtcmem_data_t)));
			return true;
		}
	}
	else {
		DEBUG_WARN ("%s do not exist", CONFIG_FILE);
		return false;
	}

	return false;
}

bool EnigmaIOTSensorClass::saveFlashData () {
	File configFile = SPIFFS.open (CONFIG_FILE, "w");
	if (!configFile) {
		DEBUG_WARN ("failed to open config file %s for writing", CONFIG_FILE);
		return false;
	}
	configFile.write ((uint8_t*)(&rtcmem_data), sizeof(rtcmem_data));
	configFile.close ();
	DEBUG_DBG ("Configuration saved to flash");
#if DEBUG_LEVEL >= VERBOSE
	dumpRtcData (&rtcmem_data/*,gateway*/);
#endif

	return true;
}


bool EnigmaIOTSensorClass::configWiFiManager (rtcmem_data_t *data) {
	AsyncWebServer server (80);
	DNSServer dns;
	char channel[4];
	itoa ((int)(data->channel),channel,10);
	DEBUG_DBG ("Channel: %s %d", channel, data->channel);
	char gateway[18] = "BE:DD:C2:24:14:97";
	//mac2str (data->gateway, gateway);
	DEBUG_DBG ("Gateway Address: %s", gateway);
	char networkKey[33] = "";
	char sleepy[5] = "10";

	AsyncWiFiManager wifiManager (&server, &dns);
	AsyncWiFiManagerParameter channelParam ("channel", "WiFi Channel", channel, 4, "required type=\"number\" min=\"0\" max=\"13\" step=\"1\"");
	AsyncWiFiManagerParameter gatewayParam ("gateway", "EnigmaIoT gateway", gateway, 18, "required type=\"text\" pattern=\"^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$\"");
	AsyncWiFiManagerParameter netKeyParam ("netkey", "NetworkKey", networkKey, 33, "required type=\"text\" maxlength=32");
	AsyncWiFiManagerParameter sleepyParam ("sleepy", "Sleep Time", sleepy, 5, "required type=\"number\" min=\"0\" max=\"13600\" step=\"1\"");

	wifiManager.addParameter (&channelParam);
	wifiManager.addParameter (&gatewayParam);
	wifiManager.addParameter (&netKeyParam);
	wifiManager.addParameter (&sleepyParam);
	wifiManager.setDebugOutput (true);
	wifiManager.setConnectTimeout (30);
	wifiManager.setBreakAfterConfig (true);
	String apname = "EnigmaIoTSensor"+String(ESP.getChipId (),16);

	boolean result = wifiManager.startConfigPortal (apname.c_str());
	if (result) {
		DEBUG_DBG ("==== Config Portal result ====");
		DEBUG_DBG ("Channel: %s", channelParam.getValue ());
		DEBUG_DBG ("Gateway: %s", gatewayParam.getValue ());
		DEBUG_DBG ("Network Key: %s", netKeyParam.getValue ());
		DEBUG_DBG ("Sleppy time: %s", sleepyParam.getValue ());

		data->lastMessageCounter = 0;
		data->channel = atoi (channelParam.getValue());
		str2mac (gatewayParam.getValue(), data->gateway);
		//memcpy (this->gateway, data->gateway, comm->getAddressLength ());
		uint8_t keySize = netKeyParam.getValueLength ();
		if (netKeyParam.getValueLength () > KEY_LENGTH)
			keySize = KEY_LENGTH;
		memcpy (data->networkKey, netKeyParam.getValue(), keySize);
		data->nodeRegisterStatus = UNREGISTERED;
		int sleepyVal = atoi (sleepyParam.getValue());
		if (sleepyVal > 0) {
			data->sleepy = true;
		}
		data->sleepTime = sleepyVal;
		data->nodeKeyValid = false;
		data->crc32 = CRC32::calculate ((uint8_t*)(data->nodeKey), sizeof (rtcmem_data_t) - sizeof (uint32_t));
	}
	return result;
}

void EnigmaIOTSensorClass::begin (Comms_halClass *comm, uint8_t *gateway, uint8_t *networkKey, bool useCounter, bool sleepy) {
    pinMode (led, OUTPUT);
    digitalWrite (led, HIGH);

	initWiFi ();
	this->comm = comm;
	
	this->useCounter = useCounter;

	node.setSleepy (sleepy);
	DEBUG_DBG ("Set %s mode: %s", node.getSleepy () ? "sleepy" : "non sleepy", sleepy ? "sleepy" : "non sleepy");

	uint8_t macAddress[6];
	
	if (wifi_get_macaddr (SOFTAP_IF, macAddress)) {
		node.setMacAddress (macAddress);
	}
	
	if (loadRTCData ()) { // If data present on RTC sensor has waked up or it is just configured, continue
#if DEBUG_LEVEL >= DBG
		char gwAddress[18];
		DEBUG_DBG ("RTC data loaded. Gateway: %s", mac2str (rtcmem_data.gateway, gwAddress));
#endif
		DEBUG_DBG ("Own address: %s", mac2str (node.getMacAddress (), gwAddress));
	} else { // No RTC data, first boot or not configured
		if (gateway && networkKey) { // If connection data has been passed to library
			DEBUG_DBG ("EnigmaIot started with config data con begin() call");
			//memcpy (this->gateway, gateway, comm->getAddressLength ()); // setGateway
			memcpy (rtcmem_data.gateway, gateway , comm->getAddressLength ()); // setGateway
			//memcpy (this->networkKey, networkKey, KEY_LENGTH);          // setNetworkKey
			memcpy (rtcmem_data.networkKey, networkKey, KEY_LENGTH);          // setNetworkKey
			rtcmem_data.nodeKeyValid = false;
			//rtcmem_data.channel = channel;
			rtcmem_data.sleepy = sleepy;
			rtcmem_data.nodeRegisterStatus = UNREGISTERED;
			// Is this needed ????
//			rtcmem_data.crc32 = CRC32::calculate ((uint8_t*)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
//			if (ESP.rtcUserMemoryWrite (0, (uint32_t*)& rtcmem_data, sizeof (rtcmem_data))) {
//#if DEBUG_LEVEL >= VERBOSE
//				DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t*)& rtcmem_data, sizeof (rtcmem_data)));
//				dumpRtcData (&rtcmem_data/*, this->gateway*/);
//#endif
//			}

		} else { // Try read from flash
			if (!SPIFFS.begin ()) {
				DEBUG_ERROR ("Error mounting flash");
				return;
			}
			if (loadFlashData ()) { // If data present on flash, read and continue
				node.setStatus (UNREGISTERED);
				DEBUG_DBG ("Flash data loaded");
			} else { // Configuration empty. Enter config AP mode
				DEBUG_DBG ("No flash data present. Starting Configuration AP");
				if (configWiFiManager (&rtcmem_data)) {// AP config data OK
					DEBUG_DBG ("Got configuration. Storing");
					rtcmem_data.crc32 = CRC32::calculate ((uint8_t*)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
					if (ESP.rtcUserMemoryWrite (0, (uint32_t*)& rtcmem_data, sizeof (rtcmem_data))) {
						DEBUG_DBG ("Write configuration data to RTC memory");
#if DEBUG_LEVEL >= VERBOSE
						DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t*)& rtcmem_data, sizeof (rtcmem_data)));
						dumpRtcData (&rtcmem_data/*, this->gateway*/);
#endif
					}
					if (!saveFlashData ()) {
						DEBUG_ERROR ("Error saving data on flash. Restarting");
					}
					ESP.restart ();
				} else { // Configuration error
					DEBUG_ERROR ("Configuration error. Restarting");
					ESP.restart ();
				}
			}
		}

	}

	comm->begin (rtcmem_data.gateway, rtcmem_data.channel);
	comm->onDataRcvd (rx_cb);
	comm->onDataSent (tx_cb);
	if (!rtcmem_data.nodeKeyValid || (rtcmem_data.nodeRegisterStatus!=REGISTERED))
		clientHello ();
	DEBUG_DBG ("Comms started");

}


void EnigmaIOTSensorClass::stop () {
	comm->stop ();
	DEBUG_DBG ("Communication layer uninitalized");
}

void EnigmaIOTSensorClass::setSleepTime (uint32_t sleepTime) {
	uint64_t maxSleepTime = (ESP.deepSleepMax () / (uint64_t)1000000);

	if (sleepTime == 0) {
		node.setSleepy (false);
		//rtcmem_data.sleepy = false;
	}
	else if (sleepTime < maxSleepTime) {
		node.setSleepy (true);
		rtcmem_data.sleepTime = sleepTime;
	}
	else {
		DEBUG_DBG ("Max sleep time is %lu", (uint32_t)maxSleepTime);
		node.setSleepy (true);
		rtcmem_data.sleepTime = (uint32_t)maxSleepTime;
	}
	this->sleepTime = (uint64_t)rtcmem_data.sleepTime * (uint64_t)1000000;
	DEBUG_DBG ("Sleep time set to %d. Sleepy mode is %s",
		rtcmem_data.sleepTime,
		node.getSleepy () ? "sleepy" : "non sleepy");
}

void EnigmaIOTSensorClass::handle () {
    static unsigned long blueOntime;

    if (led >= 0) {
        if (flashBlue) {
            blueOntime = millis ();
            digitalWrite (led, LOW);
            flashBlue = false;
        }

        if (!digitalRead (led) && millis () - blueOntime > ledOnTime) {
            digitalWrite (led, HIGH);
        }
    }

    if (sleepRequested && millis () - node.getLastMessageTime () > DOWNLINK_WAIT_TIME && node.isRegistered() && node.getSleepy()) {
		uint64_t usSleep = sleepTime / (uint64_t)1000;
		DEBUG_INFO ("Go to sleep for %lu ms", (uint32_t)(usSleep));
        ESP.deepSleep (sleepTime, RF_NO_CAL);
    }


    static time_t lastRegistration;
    status_t status = node.getStatus ();
    if (status == WAIT_FOR_SERVER_HELLO || status == WAIT_FOR_CIPHER_FINISHED) {
        if (millis () - lastRegistration > (RECONNECTION_PERIOD*10)) {
            DEBUG_DBG ("Current node status: %d", node.getStatus ());
            lastRegistration = millis ();
            node.reset ();
            clientHello ();
        }
    }


    if (node.getStatus()== UNREGISTERED) {
        if (millis () - lastRegistration > (RECONNECTION_PERIOD)) {
            DEBUG_DBG ("Current node status: %d", node.getStatus ());
            lastRegistration = millis ();
            node.reset ();
            clientHello ();
        }
    }

	if (otaRunning) {
		if (millis () - lastOTAmsg > OTA_TIMEOUT_TIME) {
			uint8_t responseBuffer[2];
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_TIMEOUT;
			if (sendData (responseBuffer, sizeof (responseBuffer), true)) {
				DEBUG_INFO ("OTA TIMEOUT");
			}
			otaRunning = false;
			restart ();
		}
	}

}

void EnigmaIOTSensorClass::rx_cb (uint8_t *mac_addr, uint8_t *data, uint8_t len) {
    EnigmaIOTSensor.manageMessage (mac_addr, data, len);
}

void EnigmaIOTSensorClass::tx_cb (uint8_t *mac_addr, uint8_t status) {
    EnigmaIOTSensor.getStatus (mac_addr, status);
}

bool EnigmaIOTSensorClass::checkCRC (const uint8_t *buf, size_t count, uint32_t *crc) {
    uint32 recvdCRC;

    memcpy (&recvdCRC, crc, sizeof (uint32_t));
    //DEBUG_VERBOSE ("Received CRC32: 0x%08X", *crc32);
    uint32_t _crc = CRC32::calculate (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X Length: %d", _crc, recvdCRC, count);
    return (_crc == recvdCRC);
}

bool EnigmaIOTSensorClass::clientHello () {
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

    Crypto.getDH1 ();
    node.setStatus (INIT);
	rtcmem_data.nodeRegisterStatus = INIT;
    uint8_t macAddress[6];
    if (wifi_get_macaddr (SOFTAP_IF, macAddress)) {
        node.setMacAddress (macAddress);
    }
    uint8_t *key = Crypto.getPubDHKey ();

    if (!key) {
        return false;
    }

    clientHello_msg.msgType = CLIENT_HELLO; // Client hello message

    CryptModule::random (clientHello_msg.iv, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (clientHello_msg.iv, IV_LENGTH));

    for (int i = 0; i < KEY_LENGTH; i++) {
        clientHello_msg.publicKey[i] = key[i];
    }

    crc32 = CRC32::calculate ((uint8_t *)&clientHello_msg, CHMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    memcpy (&(clientHello_msg.crc), &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer ((uint8_t*)&clientHello_msg, CHMSG_LEN));

    CryptModule::networkEncrypt (clientHello_msg.iv, 3, rtcmem_data.networkKey, KEY_LENGTH);

    DEBUG_VERBOSE ("Netowrk encrypted Client Hello message: %s", printHexBuffer ((uint8_t*)&clientHello_msg, CHMSG_LEN));

    node.setStatus (WAIT_FOR_SERVER_HELLO);
	rtcmem_data.nodeRegisterStatus = WAIT_FOR_SERVER_HELLO;

    DEBUG_INFO (" -------> CLIENT HELLO");

    return comm->send (rtcmem_data.gateway, (uint8_t*)&clientHello_msg, CHMSG_LEN) == 0;
}


bool EnigmaIOTSensorClass::processServerHello (const uint8_t mac[6], const uint8_t* buf, size_t count) {
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

    //uint8_t myPublicKey[KEY_LENGTH];
    uint32_t crc32;

    if (count < SHMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

    memcpy (&serverHello_msg, buf, count);

    CryptModule::networkDecrypt (serverHello_msg.iv, 3, rtcmem_data.networkKey, KEY_LENGTH);

    DEBUG_VERBOSE ("Network decrypted Server Hello message: %s", printHexBuffer ((uint8_t *)&serverHello_msg, SHMSG_LEN));

    memcpy (&crc32, &(serverHello_msg.crc), CRC_LENGTH);

    if (!checkCRC ((uint8_t*)&serverHello_msg, SHMSG_LEN - CRC_LENGTH, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    Crypto.getDH2 (serverHello_msg.publicKey);
    node.setEncryptionKey (serverHello_msg.publicKey);
	memcpy (rtcmem_data.nodeKey, node.getEncriptionKey (), KEY_LENGTH);
    DEBUG_VERBOSE ("Node key: %s", printHexBuffer (node.getEncriptionKey (), KEY_LENGTH));

    return true;
}

bool EnigmaIOTSensorClass::processCipherFinished (const uint8_t mac[6], const uint8_t* buf, size_t count) {
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

    uint16_t nodeId;
    uint32_t crc32;

    if (count < CFMSG_LEN) {
        DEBUG_WARN ("Wrong message length --> Required: %d Received: %d", CFMSG_LEN, count);
        return false;
    }

    memcpy (&cipherFinished_msg, buf, CFMSG_LEN);

    CryptModule::networkDecrypt (cipherFinished_msg.iv, 1, rtcmem_data.networkKey, KEY_LENGTH);

    DEBUG_VERBOSE ("Network decrypted Server Hello message: %s", printHexBuffer ((uint8_t *)&cipherFinished_msg, CFMSG_LEN));

    Crypto.decryptBuffer (
        (uint8_t *)&(cipherFinished_msg.nodeId),
        (uint8_t *)&(cipherFinished_msg.nodeId),
        CFMSG_LEN - IV_LENGTH - 1,
        cipherFinished_msg.iv,
        IV_LENGTH,
        node.getEncriptionKey (),
        KEY_LENGTH
    );
    DEBUG_VERBOSE ("Decripted Cipher Finished message: %s", printHexBuffer ((uint8_t *)&cipherFinished_msg, CFMSG_LEN));

    memcpy (&crc32, &(cipherFinished_msg.crc), CRC_LENGTH);

    if (!checkCRC ((uint8_t *)&cipherFinished_msg, CFMSG_LEN - 4, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

    memcpy (&nodeId, &cipherFinished_msg.nodeId, sizeof (uint16_t));
    node.setNodeId (nodeId);
    DEBUG_DBG ("Node ID: %u", node.getNodeId ());
    return true;
}

bool EnigmaIOTSensorClass::keyExchangeFinished () {
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
    uint32_t random;

    keyExchangeFinished_msg.msgType = KEY_EXCHANGE_FINISHED;

    Crypto.random (keyExchangeFinished_msg.iv, IV_LENGTH);
    DEBUG_VERBOSE ("IV: %s", printHexBuffer (keyExchangeFinished_msg.iv, IV_LENGTH));

    random = Crypto.random ();

    if (node.getSleepy ()) {
        random = random | 0x00000001U; // Signal sleepy node
		DEBUG_DBG ("Signal sleepy node");
    }
    else {
        random = random & 0xFFFFFFFEU; // Signal always awake node
		DEBUG_DBG ("Signal non sleepy node");
    }

    memcpy (&(keyExchangeFinished_msg.random), &random, RANDOM_LENGTH);

    crc32 = CRC32::calculate ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN - CRC_LENGTH);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    memcpy (&(keyExchangeFinished_msg.crc), &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Key Exchange Finished message: %s", printHexBuffer ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN));

    Crypto.encryptBuffer ((uint8_t *)&(keyExchangeFinished_msg.random), (uint8_t *)&(keyExchangeFinished_msg.random), KEFMSG_LEN - IV_LENGTH - 1, keyExchangeFinished_msg.iv, IV_LENGTH, node.getEncriptionKey (), KEY_LENGTH);

    DEBUG_VERBOSE ("Encripted Key Exchange Finished message: %s", printHexBuffer ((uint8_t *)&(keyExchangeFinished_msg), KEFMSG_LEN));

    CryptModule::networkEncrypt (keyExchangeFinished_msg.iv, 1, rtcmem_data.networkKey, KEY_LENGTH);

    DEBUG_VERBOSE ("Network encrypted Key Exchange Finished message: %s", printHexBuffer ((uint8_t *)&keyExchangeFinished_msg, KEFMSG_LEN));

    DEBUG_INFO (" -------> KEY_EXCHANGE_FINISHED");
    return comm->send (rtcmem_data.gateway, (uint8_t *)&(keyExchangeFinished_msg), KEFMSG_LEN) == 0;
}

bool EnigmaIOTSensorClass::sendData (const uint8_t *data, size_t len, bool controlMessage) {
    memcpy (dataMessageSent, data, len);
    dataMessageSentLength = len;
	node.setLastMessageTime (); // Mark message time to start RX window start

    if (node.getStatus () == REGISTERED && node.isKeyValid ()) {
		if (controlMessage) {
			DEBUG_VERBOSE ("Control message sent: %s", printHexBuffer (data, len));
		} else {
			DEBUG_VERBOSE ("Data sent: %s", printHexBuffer (data, len));
		}
        flashBlue = true;
		return dataMessage (data, len, controlMessage);
    }
    return false;
}

void EnigmaIOTSensorClass::sleep ()
{
	if (node.getSleepy ()) {
		DEBUG_DBG ("Sleep programmed for %lu ms", rtcmem_data.sleepTime * 1000);
		sleepTime = (uint64_t)rtcmem_data.sleepTime * (uint64_t)1000000;
		sleepRequested = true;
	} else {
		DEBUG_VERBOSE ("Node is non sleepy. Sleep rejected");
	}
}

bool EnigmaIOTSensorClass::dataMessage (const uint8_t *data, size_t len, bool controlMessage) {
    /*
    * --------------------------------------------------------------------------------------
    *| msgType (1) | IV (16) | length (2) | NodeId (2) | Counter (2) | Data (....) | CRC (4) |
    * --------------------------------------------------------------------------------------
    */

    uint8_t buffer[200];
    uint32_t crc32;
    uint32_t counter;
    uint16_t nodeId = node.getNodeId ();

    uint8_t *msgType_p = buffer;
    uint8_t *iv_p = buffer + 1;
    uint8_t *length_p = buffer + 1 + IV_LENGTH;
    uint8_t *nodeId_p = buffer + 1 + IV_LENGTH + sizeof (int16_t);
    uint8_t *counter_p = buffer + 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t);
    uint8_t *data_p = buffer + 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + sizeof (int16_t);

    if (!data) {
        return false;
    }

	if (controlMessage) { 
		*msgType_p = (uint8_t)CONTROL_DATA; 
	} else {
		*msgType_p = (uint8_t)SENSOR_DATA;
	}

    CryptModule::random (iv_p, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (iv_p, IV_LENGTH));

    memcpy (nodeId_p, &nodeId, sizeof (uint16_t));

	if (!controlMessage) { // Control messages do not use counter
		if (useCounter) {
			counter = node.getLastMessageCounter () + 1;
			node.setLastMessageCounter (counter);
			rtcmem_data.lastMessageCounter = counter;
		}
		else {
			counter = Crypto.random ();
		}

		memcpy (counter_p, &counter, sizeof (uint16_t));
	}

    memcpy (data_p, data, len);

    uint16_t packet_length = 1 + IV_LENGTH + sizeof (int16_t) + sizeof (int16_t) + sizeof (uint16_t) + len;

    memcpy (length_p, &packet_length, sizeof (uint16_t));

    crc32 = CRC32::calculate (buffer, packet_length);
    DEBUG_VERBOSE ("CRC32 = 0x%08X", crc32);

    // int is little indian mode on ESP platform
    uint8_t *crc_p = (uint8_t*)(buffer + packet_length);

    memcpy (crc_p, &crc32, CRC_LENGTH);

    DEBUG_VERBOSE ("Data message: %s", printHexBuffer (buffer, packet_length + CRC_LENGTH));

    uint8_t *crypt_buf = buffer + 1 + IV_LENGTH;

    size_t cryptLen = packet_length + CRC_LENGTH - 1 - IV_LENGTH;

    Crypto.encryptBuffer (crypt_buf, crypt_buf, cryptLen, iv_p, IV_LENGTH, node.getEncriptionKey (), KEY_LENGTH);

    DEBUG_VERBOSE ("Encrypted data message: %s", printHexBuffer (buffer, packet_length + CRC_LENGTH));

	if (controlMessage) {
		DEBUG_INFO (" -------> CONTROL MESSAGE");
	} else {
		DEBUG_INFO (" -------> DATA");
	}
#if DEBUG_LEVEL >= VERBOSE
	char macStr[18];
	DEBUG_DBG ("Destination address: %s", mac2str (rtcmem_data.gateway, macStr));
#endif

    if (useCounter && !otaRunning) { // RTC must not be written if OTA is running. OTA uses RTC memmory to signal 2nd firmware boot
        rtcmem_data.crc32 = CRC32::calculate ((uint8_t *)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
        if (ESP.rtcUserMemoryWrite (0, (uint32_t*)&rtcmem_data, sizeof (rtcmem_data))) {
			DEBUG_DBG ("Write configuration data to RTC memory");
#if DEBUG_LEVEL >= VERBOSE
            DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t *)&rtcmem_data, sizeof (rtcmem_data)));
			dumpRtcData (&rtcmem_data);
#endif
		}
    }

    return (comm->send (rtcmem_data.gateway, buffer, packet_length + CRC_LENGTH) == 0);
}

bool EnigmaIOTSensorClass::processGetSleepTimeCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
	uint8_t buffer[MAX_MESSAGE_LENGTH];
	uint8_t bufLength;

	DEBUG_DBG ("Get Sleep command received");
	DEBUG_VERBOSE ("%s", printHexBuffer(data, len));

	buffer[0] = control_message_type::SLEEP_ANS;
	// TODO get real sleep time
	uint32_t sleepTime = getSleepTime();
	memcpy (buffer + 1, &sleepTime, sizeof (sleepTime));
	bufLength = 5;

	if (sendData (buffer, bufLength, true)) {
		DEBUG_DBG ("Sleep time is %d seconds", sleepTime);
		DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
		return true;
	}
	else {
		DEBUG_WARN ("Error sending version response");
		return false;
	}
}

bool EnigmaIOTSensorClass::processSetSleepTimeCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
	uint8_t buffer[MAX_MESSAGE_LENGTH];
	uint8_t bufLength;

	DEBUG_DBG ("Set Sleep command received");
	DEBUG_VERBOSE ("%s", printHexBuffer (data, len));

	buffer[0] = control_message_type::SLEEP_ANS;
	
	uint32_t sleepTime;
	memcpy (&sleepTime, data + 1, sizeof (uint32_t));
	DEBUG_DBG ("Sleep time requested: %d", sleepTime);
	setSleepTime (sleepTime);
	// TODO Store config on flash if sleep time > 0
	sleepTime = getSleepTime ();
	memcpy (buffer + 1, &sleepTime, sizeof (sleepTime));
	bufLength = 5;

	if (sendData (buffer, bufLength, true)) {
		DEBUG_DBG ("Sleep time is %d seconds", sleepTime);
		DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
		return true;
	}
	else {
		DEBUG_WARN ("Error sending version response");
		return false;
	}
}


bool EnigmaIOTSensorClass::processVersionCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
	uint8_t buffer[MAX_MESSAGE_LENGTH];
	uint8_t bufLength;

	buffer[0] = control_message_type::VERSION_ANS;
	memcpy (buffer + 1, (uint8_t*)ENIGMAIOT_PROT_VERS, strlen (ENIGMAIOT_PROT_VERS));
	bufLength = strlen (ENIGMAIOT_PROT_VERS) + 1;
	DEBUG_DBG ("Version command received");
	if (sendData (buffer, bufLength, true)) {
		DEBUG_DBG ("Version is %s", ENIGMAIOT_PROT_VERS);
		DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
		return true;
	}
	else {
		DEBUG_WARN ("Error sending version response");
		return false;
	}
}

bool EnigmaIOTSensorClass::processOTACommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
	uint8_t responseBuffer[2];

	//DEBUG_VERBOSE ("Data: %s", printHexBuffer (data, len));
	uint16_t msgIdx;
	static char md5buffer[33];
	char md5calc[32];
	static uint16_t numMsgs;
	static uint32_t otaSize;
	static uint16_t oldIdx;
	static MD5Builder _md5;
	uint8_t* dataPtr = (uint8_t*)(data + 1);
	uint8_t dataLen = len - 1;

	if (dataLen < 2) {
		DEBUG_ERROR ("OTA message is too short: %u bytes", dataLen + 1);
		return false;
	}

	memcpy (&msgIdx, dataPtr, sizeof (uint16_t));
	dataPtr += sizeof (uint16_t);
	dataLen -= sizeof (uint16_t);
	DEBUG_WARN ("OTA message #%u", msgIdx);
	if (msgIdx > 0 && otaRunning) {
		if (msgIdx != (oldIdx + 1)) {
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_OUT_OF_SEQUENCE;
			sendData (responseBuffer, sizeof (responseBuffer), true);
			DEBUG_ERROR ("%u OTA messages missing before %u", msgIdx - oldIdx - 1, msgIdx);
			otaRunning = false;
			otaError = true;
			return false;
		} else {
			//Serial.println (msgIdx);
		}
	}
	oldIdx = msgIdx;
	lastOTAmsg = millis ();

	if (msgIdx == 0) {
		if (dataLen < 38) {
			DEBUG_ERROR ("OTA message #0 is too short: %u bytes", dataLen + 3);
			return false;
		}
		memcpy (&otaSize, dataPtr, sizeof (uint32_t));
		DEBUG_WARN ("OTA size: %u bytes", otaSize);
		dataPtr += sizeof (uint32_t);
		dataLen -= sizeof (uint32_t);
		memcpy (&numMsgs, dataPtr, sizeof (uint16_t));
		DEBUG_WARN ("Number of OTA messages: %u", numMsgs);
		dataPtr += sizeof (uint16_t);
		dataLen -= sizeof (uint16_t);
		memcpy (md5buffer, dataPtr, 32);
		md5buffer[32] = '\0';
		DEBUG_VERBOSE ("MD5: %s", printHexBuffer ((uint8_t *)md5buffer, 32));
		otaRunning = true;
		otaError = false;
		_md5.begin ();
		responseBuffer[0] = control_message_type::OTA_ANS;
		responseBuffer[1] = ota_status::OTA_STARTED;
		if (sendData (responseBuffer, sizeof (responseBuffer), true)) {
			DEBUG_INFO("OTA STARTED");
			restart (false); // Force unregistration after boot so that sleepy status is synchronized
			                 // on Gateway
			if (!Update.begin (otaSize)) {
				DEBUG_ERROR ("Error begginning OTA. OTA size: %u", otaSize);
				return false;
			}
			Update.runAsync (true);
			if (!Update.setMD5 (md5buffer)) {
				DEBUG_ERROR ("Error setting MD5");
				return false;
			}
		}
	} else {
		if (otaRunning) {
			static size_t totalBytes = 0;

			_md5.add (dataPtr, dataLen);
			// TODO Process OTA Update
			size_t numBytes = Update.write (dataPtr, dataLen);
			totalBytes += dataLen;
			DEBUG_WARN ("%u bytes written. Total %u", numBytes, totalBytes);
		} else {
			if (!otaError) {
				otaError = true;
				responseBuffer[0] = control_message_type::OTA_ANS;
				responseBuffer[1] = ota_status::OTA_START_ERROR;
				sendData (responseBuffer, sizeof (responseBuffer), true);
				DEBUG_ERROR ("OTA error. Message 0 not received");
			}
		}
	}

	if (msgIdx == numMsgs && otaRunning) {
		StreamString otaErrorStr;

		DEBUG_WARN ("OTA end");
		_md5.calculate ();
		DEBUG_WARN ("OTA MD5 %s", _md5.toString ().c_str ());
		_md5.getChars (md5calc);
		if (!memcmp (md5calc, md5buffer, 32)) {
			DEBUG_WARN ("OTA MD5 check OK");
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_CHECK_OK;
			sendData (responseBuffer, sizeof (responseBuffer), true);
			//delay (500);
		} else {
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_CHECK_FAIL;
			sendData (responseBuffer, sizeof (responseBuffer), true);
			DEBUG_ERROR ("OTA MD5 check failed");
		}
		Serial.print ('.');
		while (!Update.isFinished ()) {
			Serial.print ('.');
			delay (100);
		}
		Serial.println ();

		if (Update.end (true)) {
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_FINISHED;
			sendData (responseBuffer, sizeof (responseBuffer), true);
			//uint8_t otaErrorCode = Update.getError ();
			//Update.printError (otaErrorStr);
			//otaErrorStr.trim (); // remove line ending
			//DEBUG_WARN ("OTA Finished OK");
			//DEBUG_WARN("OTA eror code: %s", otaErrorStr.c_str ());
			Serial.println ("OTA OK");
			// delay (10000); // Delay does not work on lambda functions
			ESP.restart ();
			return true; // Restart does not happen inmediatelly, so code goes on
		} else {
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_CHECK_FAIL;
			sendData (responseBuffer, sizeof (responseBuffer), true);
			//uint8_t otaErrorCode = Update.getError ();
			//Update.printError (otaErrorStr);
			//otaErrorStr.trim (); // remove line ending
			//DEBUG_ERROR ("OTA Failed");
			//DEBUG_WARN ("OTA eror code: %s", otaErrorStr.c_str ());
			Serial.println ("OTA failed");
			return false;
		}
		delay (500);
		restart ();
	}

	return true;
}

void EnigmaIOTSensorClass::restart (bool reboot) {
	rtcmem_data.nodeRegisterStatus = UNREGISTERED;
	rtcmem_data.nodeKeyValid = false; // Force resync
	ESP.rtcUserMemoryWrite (0, (uint32_t*)& rtcmem_data, sizeof (rtcmem_data));
	DEBUG_DBG ("Reset configuration data in RTC memory");
	if (reboot)
		ESP.restart (); // Reboot to recover normal status
}

bool EnigmaIOTSensorClass::processControlCommand (const uint8_t* mac, const uint8_t* data, size_t len) {
	//memset (buffer, 0, MAX_MESSAGE_LENGTH);
	DEBUG_VERBOSE ("Data: %s", printHexBuffer (data, len));
	switch (data[0]){
		case control_message_type::VERSION:
			return processVersionCommand (mac, data, len);
		case control_message_type::SLEEP_GET:
			return processGetSleepTimeCommand (mac, data, len);
		case control_message_type::SLEEP_SET:
			return processSetSleepTimeCommand (mac, data, len);
		case control_message_type::OTA:
			if (processOTACommand (mac, data, len)) {
				return true;
			} else {
				DEBUG_ERROR ("Error processing OTA");
				restart ();
			}
	}
	return false;
}

bool EnigmaIOTSensorClass::processDownstreamData (const uint8_t mac[6], const uint8_t* buf, size_t count, bool control) {
    /*
    * -------------------------------------------------------------------------
    *| msgType (1) | IV (16) | length (2) | NodeId (2) | Data (....) | CRC (4) |
    * -------------------------------------------------------------------------
    */

    //uint8_t msgType_idx = 0;
    uint8_t iv_idx = 1;
    uint8_t length_idx = iv_idx + IV_LENGTH;
    uint8_t nodeId_idx = length_idx + sizeof (int16_t);
    uint8_t data_idx = nodeId_idx + sizeof (int16_t);
    uint8_t crc_idx = count - CRC_LENGTH;

    //uint8_t *iv;
    uint32_t crc32;
    //size_t lostMessages = 0;

    Crypto.decryptBuffer (
        const_cast<uint8_t *>(&buf[length_idx]),
        const_cast<uint8_t *>(&buf[length_idx]),
        count - length_idx,
        const_cast<uint8_t *>(&buf[iv_idx]),
        IV_LENGTH,
        node.getEncriptionKey (),
        KEY_LENGTH
    );
    DEBUG_VERBOSE ("Decripted downstream message: %s", printHexBuffer (buf, count));

    //memcpy (&counter, &buf[counter_idx], sizeof (uint16_t));
    //if (useCounter) {
    //    if (counter > node->getLastMessageCounter ()) {
    //        lostMessages = counter - node->getLastMessageCounter () - 1;
    //        node->setLastMessageCounter (counter);
    //    }
    //    else {
    //        return false;
    //    }
    //}

    memcpy (&crc32, &buf[crc_idx], CRC_LENGTH);

    if (!checkCRC (buf, count - 4, &crc32)) {
        DEBUG_WARN ("Wrong CRC");
        return false;
    }

	if (control) {
		DEBUG_INFO ("Control command");
		DEBUG_VERBOSE ("Data: %s", printHexBuffer (&buf[data_idx], crc_idx - data_idx));
		return processControlCommand (mac, &buf[data_idx], crc_idx - data_idx);
	}

	DEBUG_VERBOSE ("Sending data notification. Payload length: %d", crc_idx - data_idx);
    if (notifyData) {
        notifyData (mac, &buf[data_idx], crc_idx - data_idx);
    }

    return true;

}


sensorInvalidateReason_t EnigmaIOTSensorClass::processInvalidateKey (const uint8_t mac[6], const uint8_t* buf, size_t count) {
#define IKMSG_LEN 2
    if (buf && count < IKMSG_LEN) {
        return UNKNOWN_ERROR;
    }
    DEBUG_DBG ("Invalidate key request. Reason: %u", buf[1]);
    uint8_t reason = buf[1];
    return (sensorInvalidateReason_t)reason;
}

void EnigmaIOTSensorClass::manageMessage (const uint8_t *mac, const uint8_t* buf, uint8_t count) {
    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s", printHexBuffer (const_cast<uint8_t *>(buf), count));
    flashBlue = true;

    if (count <= 1) {
		DEBUG_ERROR ("Empty message received");
		return;
    }

	// All downlink messages should come from gateway
	if (memcmp (mac, rtcmem_data.gateway, comm->getAddressLength ()) != 0) {
		DEBUG_ERROR ("Message comes not from gateway");
		return;
	}

    switch (buf[0]) {
    case SERVER_HELLO:
        DEBUG_INFO (" <------- SERVER HELLO");
        if (node.getStatus () == WAIT_FOR_SERVER_HELLO) {
            if (processServerHello (mac, buf, count)) {
                keyExchangeFinished ();
                node.setStatus (WAIT_FOR_CIPHER_FINISHED);
				rtcmem_data.nodeRegisterStatus = WAIT_FOR_CIPHER_FINISHED;
            } else {
                node.reset ();
            }
        } else {
            node.reset ();
        }
        break;
    case CYPHER_FINISHED:
        DEBUG_INFO ("Cypher Finished received");
        if (node.getStatus () == WAIT_FOR_CIPHER_FINISHED) {
            DEBUG_INFO (" <------- CIPHER_FINISHED");
            if (processCipherFinished (mac, buf, count)) {
                // mark node as registered
                node.setKeyValid (true);
				rtcmem_data.nodeKeyValid = true;
                node.setKeyValidFrom (millis ());
                node.setLastMessageCounter (0);
                node.setStatus (REGISTERED);
				rtcmem_data.nodeRegisterStatus = REGISTERED;
                
                memcpy (rtcmem_data.nodeKey, node.getEncriptionKey (), KEY_LENGTH);
                rtcmem_data.lastMessageCounter = 0;
                rtcmem_data.nodeId = node.getNodeId ();
                rtcmem_data.crc32 = CRC32::calculate ((uint8_t *)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
                if (ESP.rtcUserMemoryWrite (0, (uint32_t*)&rtcmem_data, sizeof (rtcmem_data))) {
					DEBUG_DBG ("Write configuration data to RTC memory");
                    DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t *)&rtcmem_data, sizeof (rtcmem_data)));
                }

#if DEBUG_LEVEL >= INFO
                node.printToSerial (&DEBUG_ESP_PORT);
#endif
                if (notifyConnection) {
                    notifyConnection ();
                }
                // Resend last message in case of it is still pending to be sent.
                // If key expired it was successfully sent before so retransmission is not needed 
                if (invalidateReason < KEY_EXPIRED && dataMessageSentLength > 0) {
                    if (node.getStatus () == REGISTERED && node.isKeyValid ()) {
                        DEBUG_VERBOSE ("Data sent: %s", printHexBuffer (dataMessageSent, dataMessageSentLength));
                        dataMessage ((uint8_t *)dataMessageSent, dataMessageSentLength);
                        flashBlue = true;
                    }
                }
                // TODO: Store node data on EEPROM, SPIFFS or RTCMEM
                
            } else {
                node.reset ();
            }
        } else {
            node.reset ();
        }
        break;
    case INVALIDATE_KEY:
        DEBUG_INFO (" <------- INVALIDATE KEY");
        invalidateReason = processInvalidateKey (mac, buf, count);
        node.reset ();
        if (notifyDisconnection) {
            notifyDisconnection ();
        }
        break;
    case DOWNSTREAM_DATA:
        DEBUG_INFO (" <------- DOWNSTREAM DATA");
        if (processDownstreamData (mac, buf, count)) {
            DEBUG_INFO ("Downstream Data OK");
        }
		break;
	case DOWNSTREAM_CTRL_DATA:
		DEBUG_INFO (" <------- DOWNSTREAM CONTROL DATA");
		if (processDownstreamData (mac, buf, count, true)) {
			DEBUG_INFO ("Downstream Data OK");
		}
		break;
	}
}

void EnigmaIOTSensorClass::getStatus (uint8_t *mac_addr, uint8_t status) {
	if (status == 0) {
		DEBUG_DBG ("SENDStatus OK");
	} else {
		DEBUG_ERROR ("SENDStatus ERROR %d", status);
	}
}


EnigmaIOTSensorClass EnigmaIOTSensor;

