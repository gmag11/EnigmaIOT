/**
  * @file EnigmaIOTNode.cpp
  * @version 0.9.1
  * @date 28/05/2020
  * @author German Martin
  * @brief Library to build a node for EnigmaIoT system
  */

//#define ESP8266

#ifdef ESP8266
#include <Arduino.h>
#include "EnigmaIOTNode.h"
#include "timeManager.h"
#include <FS.h>
#include <MD5Builder.h>
#include <Updater.h>
#include <StreamString.h>
#include <ArduinoJson.h>

const char CONFIG_FILE[] = "/config.json";

ETSTimer ledTimer;

void EnigmaIOTNodeClass::resetConfig () {
    SPIFFS.begin ();
    SPIFFS.remove (CONFIG_FILE);
    SPIFFS.end ();
    DEBUG_WARN ("Config file %s deleted. Restarting");

    clearRTC ();
    ESP.restart ();
}

void EnigmaIOTNodeClass::setLed (uint8_t led, time_t onTime) {
    this->led = led;
    ledOnTime = onTime;
}

void EnigmaIOTNodeClass::setResetPin (int pin) {
	resetPin = pin;
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

void dumpRtcData (rtcmem_data_t* data, uint8_t* gateway = NULL) {
    Serial.println ("RTC MEM DATA:");
    if (data) {
        Serial.printf (" -- CRC: %s\n", printHexBuffer ((uint8_t*) & (data->crc32), sizeof (uint32_t)));
        Serial.printf (" -- Node Key: %s\n", printHexBuffer (data->nodeKey, KEY_LENGTH));
        Serial.printf (" -- Node key is %svalid\n", data->nodeKeyValid ? "" : "NOT ");
        Serial.printf (" -- Node status is %d: %s\n", data->nodeRegisterStatus, data->nodeRegisterStatus == REGISTERED ? "REGISTERED" : "NOT REGISTERED");
        Serial.printf (" -- Last message counter: %d\n", data->lastMessageCounter);
		Serial.printf (" -- NodeID: %d\n", data->nodeId);
        Serial.printf (" -- Channel: %d\n", data->channel);
        Serial.printf (" -- RSSI: %d\n", data->rssi);
		Serial.printf (" -- Network name: %s\n", data->networkName);
		char gwAddress[ENIGMAIOT_ADDR_LEN*3];
        Serial.printf (" -- Gateway: %s\n", mac2str (data->gateway, gwAddress));
		Serial.printf (" -- Comm errors: %d\n", data->commErrors);
        if (gateway)
            Serial.printf (" -- Gateway address: %s\n", mac2str (gateway, gwAddress));
        Serial.printf (" -- Network Key: %s\n", printHexBuffer (data->networkKey, KEY_LENGTH));
        Serial.printf (" -- Mode: %s\n", data->sleepy ? "sleepy" : "non sleepy");
    } else {
        Serial.println ("rtcmem_data pointer is NULL");
    }
}

bool EnigmaIOTNodeClass::loadRTCData () {
    if (ESP.rtcUserMemoryRead (RTC_ADDRESS, (uint32_t*)& rtcmem_data, sizeof (rtcmem_data))) {
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

bool EnigmaIOTNodeClass::loadFlashData () {
    //SPIFFS.remove (CONFIG_FILE); // Only for testing
    bool json_correct = false;

    if (SPIFFS.exists (CONFIG_FILE)) {
        DEBUG_DBG ("Opening %s file", CONFIG_FILE);
        File configFile = SPIFFS.open (CONFIG_FILE, "r");
        if (configFile) {
            DEBUG_DBG ("%s opened", CONFIG_FILE);
            size_t size = configFile.size ();
            /*if (size != sizeof (rtcmem_data)) {
                DEBUG_WARN ("Config file is corrupted. Deleting");
                SPIFFS.remove (CONFIG_FILE);
				if (SPIFFS.format ())
					DEBUG_WARN ("File system formatted");
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

            if (doc.containsKey ("networkName") && doc.containsKey ("networkKey")
                && doc.containsKey ("sleepTime")) {
                json_correct = true;
            }

            strlcpy (rtcmem_data.networkName, doc["networkName"] | "", sizeof (rtcmem_data.networkName));
            rtcmem_data.sleepTime = doc["sleepTime"].as<int> ();
            rtcmem_data.sleepy = !(rtcmem_data.sleepTime == 0);
            
            memset (rtcmem_data.networkKey, 0, KEY_LENGTH);
            strlcpy ((char*)rtcmem_data.networkKey, doc["networkKey"] | "", sizeof (rtcmem_data.networkKey));
            DEBUG_DBG ("Network Key dump: %s", printHexBuffer (rtcmem_data.networkKey, KEY_LENGTH));
            strlcpy (networkKey, doc["networkKey"] | "", sizeof (networkKey));

            uint8_t gwAddr[ENIGMAIOT_ADDR_LEN];
            char gwAddrStr[ENIGMAIOT_ADDR_LEN * 3];
            if (doc.containsKey ("gateway")) {
                strlcpy (gwAddrStr, doc["gateway"], sizeof (gwAddrStr));
                str2mac (gwAddrStr, gwAddr);
                memcpy (rtcmem_data.gateway, gwAddr, 6);
            }

            configFile.close ();
            // TODO: Check CRC
            if (json_correct) {
                DEBUG_VERBOSE ("Configuration successfuly read");
            }

            DEBUG_DBG ("==== EnigmaIOT Node Configuration ====");
            DEBUG_DBG ("Network name: %s", rtcmem_data.networkName);
            DEBUG_DBG ("Sleep time: %u", rtcmem_data.sleepTime);
            DEBUG_DBG ("Gateway: %s", gwAddrStr);
            DEBUG_VERBOSE ("Network key: %s", rtcmem_data.networkKey);
            CryptModule::getSHA256 (rtcmem_data.networkKey, KEY_LENGTH);
            DEBUG_VERBOSE ("Raw Network key: %s", printHexBuffer (rtcmem_data.networkKey, KEY_LENGTH));

            String output;
            serializeJsonPretty (doc, output);

            DEBUG_DBG ("JSON file %s", output.c_str ());

            //return true;
        }
    } else {
        DEBUG_WARN ("%s do not exist", CONFIG_FILE);
        //return false;
    }

    return json_correct;
}

bool EnigmaIOTNodeClass::saveFlashData (bool fsOpen) {
	if (configCleared)
		return false;

	if (!fsOpen)
		SPIFFS.begin ();

    File configFile = SPIFFS.open (CONFIG_FILE, "w");
    if (!configFile) {
        DEBUG_WARN ("failed to open config file %s for writing", CONFIG_FILE);
        return false;
    }

    DynamicJsonDocument doc (512);

    char gwAddrStr[ENIGMAIOT_ADDR_LEN * 3];
    mac2str (rtcmem_data.gateway, gwAddrStr);

    doc["networkName"] = rtcmem_data.networkName;
    doc["networkKey"] = networkKey;
    DEBUG_INFO ("NetworkKey --> %s", networkKey);
    doc["sleepTime"] = rtcmem_data.sleepTime;
    doc["gateway"] = gwAddrStr;

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

    configFile.close ();
    DEBUG_DBG ("Configuration saved to flash. %u bytes", size);
#if DEBUG_LEVEL >= DBG
    dumpRtcData (&rtcmem_data);
#endif	
	if (!fsOpen)
		SPIFFS.end ();
    return true;
}

bool EnigmaIOTNodeClass::saveRTCData () {
	if (configCleared)
		return false;
	rtcmem_data.crc32 = calculateCRC32 ((uint8_t*)rtcmem_data.nodeKey, sizeof (rtcmem_data) - sizeof (uint32_t));
	if (ESP.rtcUserMemoryWrite (RTC_ADDRESS, (uint32_t*)& rtcmem_data, sizeof (rtcmem_data))) {
		DEBUG_DBG ("Write configuration data to RTC memory");
#if DEBUG_LEVEL >= VERBOSE
		DEBUG_VERBOSE ("Write RTCData: %s", printHexBuffer ((uint8_t*)& rtcmem_data, sizeof (rtcmem_data)));
		dumpRtcData (&rtcmem_data/*, this->gateway*/);
#endif
		return true;
	}
	return false;
}

void EnigmaIOTNodeClass::clearFlash () {
	if (!SPIFFS.begin ()) {
		DEBUG_ERROR ("Error on SPIFFS.begin()");
	}
	if (SPIFFS.remove (CONFIG_FILE)) {
		DEBUG_DBG ("%s deleted", CONFIG_FILE);
	} else {
		DEBUG_ERROR ("Error on SPIFFS.remove(\"%s\")", CONFIG_FILE);
	}
	SPIFFS.end ();
}

bool EnigmaIOTNodeClass::configWiFiManager (rtcmem_data_t* data) {
    AsyncWebServer server (80);
    DNSServer dns;

    char networkKey[33] = "";
    char sleepy[5] = "10";
	char networkName[NETWORK_NAME_LENGTH] = "";

    wifiManager = new AsyncWiFiManager (&server, &dns);

    AsyncWiFiManagerParameter networkNameParam ("netname", "Network name", networkName, (int)NETWORK_NAME_LENGTH, "required type=\"text\" maxlength=20");
	AsyncWiFiManagerParameter netKeyParam ("netkey", "NetworkKey", networkKey, 33, "required type=\"password\" maxlength=32");
    AsyncWiFiManagerParameter sleepyParam ("sleepy", "Sleep Time", sleepy, 5, "required type=\"number\" min=\"0\" max=\"13600\" step=\"1\"");

    wifiManager->addParameter (&networkNameParam);
    wifiManager->addParameter (&netKeyParam);
    wifiManager->addParameter (&sleepyParam);

    if (notifyWiFiManagerStarted) {
        notifyWiFiManagerStarted ();
    }

    wifiManager->setDebugOutput (true);
    wifiManager->setConnectTimeout (30);
    wifiManager->setBreakAfterConfig (true);
	wifiManager->setTryConnectDuringConfigPortal (false);
    String apname = "EnigmaIoTNode" + String (ESP.getChipId (), 16);

    boolean result = wifiManager->startConfigPortal (apname.c_str (), NULL);
    if (true /*result*/) {
        DEBUG_DBG ("==== Config Portal result ====");

        DEBUG_DBG ("Network Name: %s", networkNameParam.getValue ());
        DEBUG_DBG ("Network Key: %s", netKeyParam.getValue ());
        DEBUG_DBG ("Sleppy time: %s", sleepyParam.getValue ());

        data->lastMessageCounter = 0;

        memcpy (data->networkName, networkNameParam.getValue (), networkNameParam.getValueLength ());

        DEBUG_DBG ("Stored network name: %s", data->networkName);
        uint8_t keySize = netKeyParam.getValueLength ();
        if (netKeyParam.getValueLength () > KEY_LENGTH)
            keySize = KEY_LENGTH;
        memset (this->networkKey, 0, KEY_LENGTH);
        memcpy (this->networkKey, netKeyParam.getValue (), keySize);
        memcpy (data->networkKey, netKeyParam.getValue (), keySize);

        CryptModule::getSHA256 (data->networkKey, KEY_LENGTH);
        DEBUG_DBG ("Calculated network key: %s", printHexBuffer (data->networkKey, KEY_LENGTH));
        DEBUG_DBG ("User network key: %s", this->networkKey, KEY_LENGTH);
        data->nodeRegisterStatus = UNREGISTERED;
        int sleepyVal = atoi (sleepyParam.getValue ());
        if (sleepyVal > 0) {
            data->sleepy = true;
        }
        data->sleepTime = sleepyVal;
        data->nodeKeyValid = false;
        data->crc32 = calculateCRC32 ((uint8_t*)(data->nodeKey), sizeof (rtcmem_data_t) - sizeof (uint32_t));
    }

    if (notifyWiFiManagerExit) {
        notifyWiFiManagerExit (true /*result*/);
    }

    return result;
}

void startFlash (time_t period) {
    ets_timer_disarm (&ledTimer);
    ets_timer_arm_new (&ledTimer, period, true, true);
}

void stopFlash () {
    ets_timer_disarm (&ledTimer);
}

void flashLed (void* led) {
    digitalWrite (*(int*)led, !digitalRead (*(int*)led));
}

void EnigmaIOTNodeClass::startIdentifying (time_t period) {
    identifyStart = millis ();
    indentifying = true;
    startFlash (period);
}

void EnigmaIOTNodeClass::stopIdentifying () {
    indentifying = false;
    stopFlash ();
}

void EnigmaIOTNodeClass::checkResetButton () {
	if (resetPin > 0) {
		pinMode (resetPin, INPUT_PULLUP);
		digitalWrite (led, LOW); // Turn on LED
		if (digitalRead (resetPin) == LOW) { // If pin is grounded
			time_t resetPinGrounded = millis ();
			while (digitalRead (resetPin) == LOW) {
				if (millis () - resetPinGrounded > RESET_PIN_DURATION) {
					DEBUG_WARN ("Produce reset");
					digitalWrite (led, HIGH); // Turn off LED
					startFlash (50);
					clearFlash ();
					clearRTC ();
					delay (5000);
					ESP.restart ();
				}
				delay (50);
			}
		}
	}
}

void EnigmaIOTNodeClass::begin (Comms_halClass* comm, uint8_t* gateway, uint8_t* networkKey, bool useCounter, bool sleepy) {
    pinMode (led, OUTPUT);
	ets_timer_setfn (&ledTimer, flashLed, (void*)& led);

	checkResetButton ();

    digitalWrite (led, HIGH);

    this->comm = comm;

    this->useCounter = useCounter;

	node.setInitAsSleepy (sleepy);
	node.setSleepy (sleepy);
    DEBUG_DBG ("Set %s mode: %s", node.getSleepy () ? "sleepy" : "non sleepy", sleepy ? "sleepy" : "non sleepy");

    uint8_t macAddress[ENIGMAIOT_ADDR_LEN];

    if (wifi_get_macaddr (STATION_IF, macAddress)) {
        node.setMacAddress (macAddress);
    }

    if (loadRTCData () && rtcmem_data.commErrors < COMM_ERRORS_BEFORE_SCAN) { // If data present on RTC node has waked up or it is just configured, continue
#if DEBUG_LEVEL >= DBG
        char gwAddress[ENIGMAIOT_ADDR_LEN * 3];
        DEBUG_DBG ("RTC data loaded. Gateway: %s", mac2str (rtcmem_data.gateway, gwAddress));
#endif
        DEBUG_DBG ("Own address: %s", mac2str (node.getMacAddress (), gwAddress));
    } else { // No RTC data, first boot or not configured
        if (gateway && networkKey) { // If connection data has been passed to library
            DEBUG_DBG ("EnigmaIot started with config data con begin() call");
            memcpy (rtcmem_data.gateway, gateway, comm->getAddressLength ()); // setGateway
            memcpy (rtcmem_data.networkKey, networkKey, KEY_LENGTH);          // setNetworkKey
            CryptModule::getSHA256 (rtcmem_data.networkKey, KEY_LENGTH);
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
				if (SPIFFS.format ()) {
					DEBUG_INFO ("SPIFFS formatted");
				} else {
					DEBUG_ERROR ("Error formatting SPIFFS");
				}
				delay (2500);
				ESP.restart ();
                //return;
            }
            if (loadFlashData ()) { // If data present on flash, read and continue
                node.setStatus (UNREGISTERED);
                DEBUG_DBG ("Flash data loaded");
                uint8_t prevGwAddr[ENIGMAIOT_ADDR_LEN];
                memcpy (prevGwAddr, rtcmem_data.gateway, 6);
				if (searchForGateway (&rtcmem_data),true) {
					//DEBUG_DBG ("Found gateway. Storing");
					rtcmem_data.commErrors = 0;
					//if (!saveRTCData()) {
					//	DEBUG_ERROR ("Error saving data on RTC");
					//}
     //               //disabled
     //               if (memcmp (prevGwAddr, rtcmem_data.gateway, 6)) {
     //                   if (!saveFlashData ()) {
     //                       DEBUG_ERROR ("Error saving data on flash");
     //                   }
     //               }

				}
            } else { // Configuration empty. Enter config AP mode
                DEBUG_DBG ("No flash data present. Starting Configuration AP");
                configWiFiManager (&rtcmem_data);
                if (true) {// AP not needed. Assume always valid
                    DEBUG_DBG ("Got configuration. Searching for Gateway");
                    if (!searchForGateway (&rtcmem_data),true) {
                        DEBUG_DBG ("Found EnigmaIOT Gateway. Storing configuration");
						if (!saveFlashData (true)) {
							DEBUG_ERROR ("Error saving data on flash");
						}
						SPIFFS.end ();
						ESP.restart ();
						return;
					}

					//if (!saveRTCData ()) {
					//	DEBUG_ERROR ("Error saving data on RTC");
					//}
     //               if (!saveFlashData (true)) {
     //                   DEBUG_ERROR ("Error saving data on flash. Restarting");
     //               }
                    SPIFFS.end ();
                    ESP.restart ();
                } else { // Configuration error
                    DEBUG_ERROR ("Configuration error. Restarting");
                    ESP.restart ();
                }
            }
        }

    }

	initWiFi (rtcmem_data.channel);
    comm->begin (rtcmem_data.gateway, rtcmem_data.channel);
    comm->onDataRcvd (rx_cb);
    comm->onDataSent (tx_cb);
	//if (!rtcmem_data.nodeKeyValid || (rtcmem_data.nodeRegisterStatus != REGISTERED))
		//clientHello ();
	//else if (!node.getSleepy () && node.isRegistered ()){
		//clockRequest ();
	//}
	wifi_set_channel (rtcmem_data.channel);
    DEBUG_DBG ("Comms started. Channel %u", rtcmem_data.channel);

}

bool EnigmaIOTNodeClass::searchForGateway (rtcmem_data_t* data, bool shouldStoreData) {
	DEBUG_DBG ("Searching for AP %s", data->networkName);

	//WiFi.mode (WIFI_STA);
	int numWifi = WiFi.scanNetworks (false, false, 0, (uint8_t*)(data->networkName));
	time_t scanStarted = millis ();
	while (!(WiFi.scanComplete () || (millis () - scanStarted) > 1500)) {
#if DEBUG_LEVEL >= DBG
		delay (250);
		Serial.printf ("%d.", millis () - scanStarted);
#else
		delay (50);
#endif
	}
    
    uint8_t prevGwAddr[ENIGMAIOT_ADDR_LEN];
    memcpy (prevGwAddr, data->gateway, 6);

	if (numWifi > 0) {
		DEBUG_INFO ("Gateway %s found: %d", data->networkName, numWifi);
		DEBUG_INFO ("BSSID: %s", WiFi.BSSIDstr (0).c_str());
		DEBUG_INFO ("Channel: %d", WiFi.channel (0));
		DEBUG_INFO ("RSSI: %d", WiFi.RSSI (0));
		// TODO: In future, to manage redundancy select higher RSSI gateway
		data->channel = WiFi.channel (0); // It is done here, maybe
		data->rssi = WiFi.RSSI (0);
		memcpy (data->gateway, WiFi.BSSID (0), 6);

		if (shouldStoreData) {
            DEBUG_DBG ("Found gateway. Storing");
			if (!saveRTCData ()) {
				DEBUG_ERROR ("Error saving data on RTC");
			}
            if (memcmp (prevGwAddr, data->gateway, 6)) {
                if (!saveFlashData ()) {
                    DEBUG_ERROR ("Error saving data on flash");
                }
            }
        }

		WiFi.scanDelete ();
		//WiFi.mode (WIFI_AP);
		//WiFi.softAPdisconnect ();

		wifi_set_channel (data->channel);

		requestReportRSSI = true;
		return true;
	}
	DEBUG_WARN ("Gateway %s not found", data->networkName);
	return false;
}


void EnigmaIOTNodeClass::stop () {
    comm->stop ();
    DEBUG_DBG ("Communication layer uninitalized");
}

void EnigmaIOTNodeClass::setSleepTime (uint32_t sleepTime) {
    if (node.getInitAsSleepy()) {
        uint64_t maxSleepTime = (ESP.deepSleepMax () / (uint64_t)1000000);

        if (sleepTime == 0) {
            node.setSleepy (false);
            //rtcmem_data.sleepy = false;
        } else if (sleepTime < maxSleepTime) {
            node.setSleepy (true);
            rtcmem_data.sleepTime = sleepTime;
        } else {
            DEBUG_DBG ("Max sleep time is %lu", (uint32_t)maxSleepTime);
            node.setSleepy (true);
            rtcmem_data.sleepTime = (uint32_t)maxSleepTime;
        }
        this->sleepTime = (uint64_t)rtcmem_data.sleepTime * (uint64_t)1000000;
        DEBUG_DBG ("Sleep time set to %d. Sleepy mode is %s",
                   rtcmem_data.sleepTime,
                   node.getSleepy () ? "sleepy" : "non sleepy");

    } else {
        DEBUG_WARN ("Cannot set sleep time to %u seconds as this node started as non sleepy", sleepTime);
    }
}

bool EnigmaIOTNodeClass::reportRSSI () {
	uint8_t buffer[MAX_MESSAGE_LENGTH];
	uint8_t bufLength;

	DEBUG_DBG ("Report RSSI and channel");

	buffer[0] = control_message_type::RSSI_ANS;
	buffer[1] = rtcmem_data.rssi;
	buffer[2] = rtcmem_data.channel;
	bufLength = 3;

	if (sendData (buffer, bufLength, true)) {
		DEBUG_DBG ("Sleep time is %d seconds", sleepTime/1000000);
		DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
		return true;
	} else {
		DEBUG_WARN ("Error sending version response");
		return false;
	}
}

void EnigmaIOTNodeClass::handle () {
    static unsigned long blueOntime;

	// Locate gateway address, channel and rssi
	if (requestSearchGateway) {
		requestSearchGateway = false;
		requestReportRSSI = true;
		searchForGateway (&rtcmem_data, true);
	}

	// Report RSSI to gateway
	if (requestReportRSSI && node.isRegistered ()) {
		requestReportRSSI = false;
		reportRSSI ();
	}

	// Flash led if programmed (when data is transferred)
    if (led >= 0) {
        if (flashBlue) {
            blueOntime = millis ();
            digitalWrite (led, LOW);
            flashBlue = false;
        }

        if (!indentifying) {
            if (!digitalRead (led) && millis () - blueOntime > ledOnTime) {
                digitalWrite (led, HIGH);
            }
        }
    }

    // Check if this should go to sleep
    if (node.getSleepy ()) {
        if (sleepRequested && millis () - node.getLastMessageTime () > DOWNLINK_WAIT_TIME && node.isRegistered () && !indentifying) {
            uint64_t usSleep = sleepTime / (uint64_t)1000;
            DEBUG_INFO ("Go to sleep for %lu ms", (uint32_t)(usSleep));
            ESP.deepSleep (sleepTime);
        }
    }

    // Check registration timeout
    static time_t lastRegistration = millis();
    status_t status = node.getStatus ();
    if (status == WAIT_FOR_SERVER_HELLO /*|| status == WAIT_FOR_CIPHER_FINISHED*/) {
		if (node.getSleepy ()) { // Sleep after registration timeout
			if (millis () - node.getLastMessageTime () > RECONNECTION_PERIOD) {
				DEBUG_WARN ("Current node status: %d", node.getStatus ());
				node.reset ();
				rtcmem_data.nodeRegisterStatus = UNREGISTERED;
				
				if (!saveRTCData ()) {
					DEBUG_ERROR ("Error saving data on RTC");
				}

				DEBUG_INFO ("Registration timeout. Go to sleep for %lu ms", (uint32_t)(RECONNECTION_PERIOD * 4));
				uint32_t rnd = Crypto.random (PRE_REG_DELAY * 1000); // nanoseconds
				ESP.deepSleep (RECONNECTION_PERIOD * 4000 + rnd, RF_NO_CAL);
			}
		} else { // Retry registration
			if (millis () - node.getLastMessageTime () > RECONNECTION_PERIOD * 5) {
				DEBUG_INFO ("Current node status: %d", node.getStatus ());
				node.reset ();
				node.setLastMessageTime (); // Set wait time start
			}
		}
    }

    // Retry registration
    if (node.getStatus () == UNREGISTERED) {
        if (millis () - lastRegistration > RECONNECTION_PERIOD) {
            DEBUG_DBG ("Current node status: %d", node.getStatus ());
            lastRegistration = millis (); // Set wait time start
            node.reset ();
			uint32_t rnd = Crypto.random (PRE_REG_DELAY);
			DEBUG_INFO ("Random delay (%u)", rnd);
			delay (1500 + rnd);
            clientHello ();
			delay (1500 + Crypto.random (POST_REG_DELAY)); // Wait for Server Hello
        }
    }

    // Check OTA update timeout
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

    // Check identifying LED timeout
    if (indentifying) {
        if (millis () - identifyStart > IDENTIFY_TIMEOUT) {
            stopIdentifying ();
            digitalWrite (led, HIGH);
        }
    }

	//if (!node.getSleepy () && node.isRegistered () && !node.hasClockSync())
	//	clockRequest ();

    // Check time sync timeout
	if (clockSyncEnabled) {
		static time_t lastTimeSync;
		if (!node.getSleepy () && node.isRegistered ()) {
			if ((millis () - lastTimeSync > timeSyncPeriod)) {
				lastTimeSync = millis ();
				DEBUG_DBG ("Clock Request");
				clockRequest ();
			}
		}
	}

	// Check restart
	if (shouldRestart) {
		static unsigned long retartRequest = millis();
		if (millis () - retartRequest > 2500) {
			DEBUG_WARN ("Restart");
			ESP.restart ();
		}
	}

	// In case of comm errors search for gateway again
	if (CHECK_COMM_ERRORS && (rtcmem_data.commErrors >= COMM_ERRORS_BEFORE_SCAN)) {
		if (!gatewaySearchStarted) {
			gatewaySearchStarted = true;
			
			if (searchForGateway (&rtcmem_data),true) {
				//DEBUG_DBG ("Found gateway. Storing");
				rtcmem_data.commErrors = 0;
				//if (!saveRTCData ()) {
				//	DEBUG_ERROR ("Error saving data on RTC");
				//}
                //disabled
				//if (!saveFlashData ()) {
				//	DEBUG_ERROR ("Error saving data on flash.");
				//}

			}
		}
	}

}

void EnigmaIOTNodeClass::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
    EnigmaIOTNode.manageMessage (mac_addr, data, len);
}

void EnigmaIOTNodeClass::tx_cb (uint8_t* mac_addr, uint8_t status) {
    EnigmaIOTNode.getStatus (mac_addr, status);
}

bool EnigmaIOTNodeClass::checkCRC (const uint8_t* buf, size_t count, uint32_t* crc) {
    uint32 recvdCRC;

    memcpy (&recvdCRC, crc, sizeof (uint32_t));
    //DEBUG_VERBOSE ("Received CRC32: 0x%08X", *crc32);
    uint32_t _crc = calculateCRC32 (buf, count);
    DEBUG_VERBOSE ("CRC32 =  Calc: 0x%08X Recvd: 0x%08X Length: %d", _crc, recvdCRC, count);
    return (_crc == recvdCRC);
}

bool EnigmaIOTNodeClass::clientHello () {
    /*
    * -------------------------------------------------------
    *| msgType (1) | random (12) | DH Kmaster (32) | Tag (16) |
    * -------------------------------------------------------
    */

    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
        uint8_t iv[IV_LENGTH];
        uint8_t publicKey[KEY_LENGTH];
		uint32_t random;
        uint8_t tag[TAG_LENGTH];
    } clientHello_msg;

#define CHMSG_LEN sizeof(clientHello_msg)

	invalidateReason = UNKNOWN_ERROR; // reset any previous force disconnect

    Crypto.getDH1 ();
    node.setStatus (INIT);
    rtcmem_data.nodeRegisterStatus = INIT;
    uint8_t macAddress[ENIGMAIOT_ADDR_LEN];
    if (wifi_get_macaddr (STATION_IF, macAddress)) {
        node.setMacAddress (macAddress);
    }

    uint8_t* key = Crypto.getPubDHKey ();

    if (!key) {
        DEBUG_ERROR ("Error calculating public ECDH key");
        return false;
    }

    clientHello_msg.msgType = CLIENT_HELLO; // Client hello message

    CryptModule::random (clientHello_msg.iv, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (clientHello_msg.iv, IV_LENGTH));

    for (int i = 0; i < KEY_LENGTH; i++) {
        clientHello_msg.publicKey[i] = key[i];
    }

	uint32_t random;
	random = Crypto.random ();

	if (node.getSleepy ()) {
		random = random | 0x00000001U; // Signal sleepy node
		DEBUG_DBG ("Signal sleepy node");
	} else {
		random = random & 0xFFFFFFFEU; // Signal always awake node
		DEBUG_DBG ("Signal non sleepy node");
	}

	memcpy (&(clientHello_msg.random), &random, RANDOM_LENGTH);

    DEBUG_VERBOSE ("Client Hello message: %s", printHexBuffer ((uint8_t*)& clientHello_msg, CHMSG_LEN - TAG_LENGTH));

    uint8_t addDataLen = CHMSG_LEN - TAG_LENGTH - sizeof (uint32_t) - KEY_LENGTH;
    uint8_t aad[AAD_LENGTH + addDataLen];

    memcpy (aad, (uint8_t*)&clientHello_msg, addDataLen); // Copy message upto iv

    // Copy 8 last bytes from NetworkKey
    memcpy (aad + addDataLen, rtcmem_data.networkKey + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

    if (!CryptModule::encryptBuffer (clientHello_msg.publicKey, KEY_LENGTH + sizeof(uint32_t), // Encrypt only from public key
                                     clientHello_msg.iv, IV_LENGTH,
                                     rtcmem_data.networkKey, KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
                                     aad, sizeof (aad), clientHello_msg.tag, TAG_LENGTH)) {
        DEBUG_ERROR ("Error during encryption");
        return false;
    }

    DEBUG_VERBOSE ("Encrypted Client Hello message: %s", printHexBuffer ((uint8_t*)& clientHello_msg, CHMSG_LEN));

    node.setStatus (WAIT_FOR_SERVER_HELLO);
    rtcmem_data.nodeRegisterStatus = WAIT_FOR_SERVER_HELLO;

    DEBUG_INFO (" -------> CLIENT HELLO");

    return comm->send (rtcmem_data.gateway, (uint8_t*)& clientHello_msg, CHMSG_LEN) == 0;
}

bool EnigmaIOTNodeClass::clockRequest () {
    struct  __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		clock_t t1;
        uint8_t tag[TAG_LENGTH];
    } clockRequest_msg;

    static const uint8_t CRMSG_LEN = sizeof(clockRequest_msg);

    clockRequest_msg.msgType = CLOCK_REQUEST;

	CryptModule::random (clockRequest_msg.iv, IV_LENGTH);

	DEBUG_VERBOSE ("IV: %s", printHexBuffer (clockRequest_msg.iv, IV_LENGTH));


    node.t1 = TimeManager.setOrigin();

    memcpy(&(clockRequest_msg.t1),&(node.t1),sizeof(clock_t));

	DEBUG_VERBOSE ("Clock Request message: %s", printHexBuffer ((uint8_t*)& clockRequest_msg, CRMSG_LEN - TAG_LENGTH));
	DEBUG_DBG ("T1: %u", node.t1);
	
	uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, (uint8_t*)& clockRequest_msg, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node.getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	if (!CryptModule::encryptBuffer ((uint8_t*)&(clockRequest_msg.t1), sizeof (clock_t), // Encrypt only from public key
									 clockRequest_msg.iv, IV_LENGTH,
									 node.getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), clockRequest_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during encryption");
		return false;
	}

	DEBUG_VERBOSE ("Encrypted Clock Request message: %s", printHexBuffer ((uint8_t*)&clockRequest_msg, CRMSG_LEN));

	DEBUG_INFO (" -------> CLOCK REQUEST");

	node.setLastMessageTime ();

	return comm->send (rtcmem_data.gateway, (uint8_t*)& clockRequest_msg, CRMSG_LEN) == 0;

}

bool EnigmaIOTNodeClass::processClockResponse (const uint8_t* mac, const uint8_t* buf, size_t count) {
    struct __attribute__ ((packed, aligned (1))) {
        uint8_t msgType;
		uint8_t iv[IV_LENGTH];
		clock_t t2;
        clock_t t3;
        uint8_t tag[TAG_LENGTH];
    } clockResponse_msg;

#define CRSMSG_LEN sizeof(clockResponse_msg)

	memcpy (&clockResponse_msg, buf, count);

	uint8_t addDataLen = 1 + IV_LENGTH;
	uint8_t aad[AAD_LENGTH + addDataLen];

	memcpy (aad, buf, addDataLen); // Copy message upto iv

	// Copy 8 last bytes from NetworkKey
	memcpy (aad + addDataLen, node.getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

	uint8_t packetLen = count - TAG_LENGTH;

	if (!CryptModule::decryptBuffer ((uint8_t*)&(clockResponse_msg.t2), sizeof(clock_t) << 1, // Decrypt from t2, 8 bytes
									 clockResponse_msg.iv, IV_LENGTH,
									 node.getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
									 aad, sizeof (aad), clockResponse_msg.tag, TAG_LENGTH)) {
		DEBUG_ERROR ("Error during decryption");
		return false;
	}

	DEBUG_VERBOSE ("Decripted Clock Response message: %s", printHexBuffer ((uint8_t*)&clockResponse_msg, packetLen));


	node.t2 = clockResponse_msg.t2;
	node.t3 = clockResponse_msg.t3;
	node.t4 = TimeManager.clock ();

    if (count < CRSMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

    memcpy (&clockResponse_msg, buf, count);

    time_t offset = TimeManager.adjustTime(node.t1, node.t2, node.t3, node.t4);

	if (offset < MIN_SYNC_ACCURACY && offset > (MIN_SYNC_ACCURACY * -1) ) {
		timeSyncPeriod = TIME_SYNC_PERIOD;
	} else {
		timeSyncPeriod = QUICK_SYNC_TIME;
	}
	DEBUG_VERBOSE ("Clock Response message: %s", printHexBuffer ((uint8_t*)& clockResponse_msg, CRSMSG_LEN - TAG_LENGTH));
	
	DEBUG_DBG ("T1: %u", node.t1);
	DEBUG_DBG ("T2: %u", node.t2);
	DEBUG_DBG ("T3: %u", node.t3);
	DEBUG_DBG ("T4: %u", node.t4);
	DEBUG_DBG ("Offest adjusted to %d ms, Roundtrip delay is %d", offset, TimeManager.getDelay ());
}

time_t EnigmaIOTNodeClass::clock () {
	if (node.getInitAsSleepy ()) {
		return millis ();
	} else {
		return TimeManager.clock ();
	}

}

bool EnigmaIOTNodeClass::hasClockSync () {
	if (!node.getInitAsSleepy ())
		return TimeManager.isTimeAdjusted ();
	else
		return false;
}

bool EnigmaIOTNodeClass::processServerHello (const uint8_t* mac, const uint8_t* buf, size_t count) {
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

	uint16_t nodeId;

    if (count < SHMSG_LEN) {
        DEBUG_WARN ("Message too short");
        return false;
    }

    memcpy (&serverHello_msg, buf, count);

    uint8_t addDataLen = SHMSG_LEN - TAG_LENGTH - sizeof (uint32_t) - sizeof (uint16_t) - KEY_LENGTH;
    uint8_t aad[AAD_LENGTH + addDataLen];

    memcpy (aad, (uint8_t*)&serverHello_msg, addDataLen); // Copy message upto iv

    // Copy 8 last bytes from NetworkKey
    memcpy (aad + addDataLen, rtcmem_data.networkKey + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

    if (!CryptModule::decryptBuffer (serverHello_msg.publicKey, KEY_LENGTH + sizeof (uint16_t) + sizeof (uint32_t),
                                     serverHello_msg.iv, IV_LENGTH,
                                     rtcmem_data.networkKey, KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
                                     aad, sizeof (aad), serverHello_msg.tag, TAG_LENGTH)) {
        DEBUG_ERROR ("Error during decryption");
        return false;
    }

    DEBUG_VERBOSE ("Decrypted Server Hello message: %s", printHexBuffer ((uint8_t*)& serverHello_msg, SHMSG_LEN - TAG_LENGTH));

    bool cError = Crypto.getDH2 (serverHello_msg.publicKey);

    if (!cError) {
        DEBUG_ERROR ("DH2 error");
        return false;
    }

	memcpy (&nodeId, &serverHello_msg.nodeId, sizeof (uint16_t));
	node.setNodeId (nodeId);
	rtcmem_data.nodeKeyValid = nodeId;
	DEBUG_DBG ("Node ID: %u", node.getNodeId ());

    node.setEncryptionKey (CryptModule::getSHA256 (serverHello_msg.publicKey, KEY_LENGTH));
    memcpy (rtcmem_data.nodeKey, node.getEncriptionKey (), KEY_LENGTH);
    DEBUG_INFO ("Node key: %s", printHexBuffer (node.getEncriptionKey (), KEY_LENGTH));

    return true;
}

bool EnigmaIOTNodeClass::sendData (const uint8_t* data, size_t len, bool controlMessage, bool encrypt, nodePayloadEncoding_t payloadType) {
    if (!controlMessage) {
        memcpy (dataMessageSent, data, len);
        dataMessageSentLength = len;
        dataMessageEncrypt = encrypt;
        dataMessageSendPending = true;
        dataMessageSendEncoding = payloadType;
    }
    node.setLastMessageTime (); // Mark message time to start RX window start

    if (node.getStatus () == REGISTERED && node.isKeyValid ()) {
        if (controlMessage) {
            DEBUG_VERBOSE ("Control message sent: %s", printHexBuffer (data, len));
        } else {
            DEBUG_VERBOSE ("%s data sent: %s", encrypt?"Encrypted":"Unencrypted", printHexBuffer (data, len));
        }
        flashBlue = true;
		if (dataMessage (data, len, controlMessage, encrypt, payloadType)) {
			//dataMessageSentLength = 0;
            dataMessageSendPending = false; // Data sent. This setting can still be overriden by invalidateCommand
			return true;
		} else
			return false;

    }
    return false;
}

void EnigmaIOTNodeClass::sleep () {
    if (node.getSleepy ()) {
        DEBUG_DBG ("Sleep programmed for %lu ms", rtcmem_data.sleepTime * 1000);
        sleepTime = (uint64_t)rtcmem_data.sleepTime * (uint64_t)1000000;
        sleepRequested = true;
    } else {
        DEBUG_VERBOSE ("Node is non sleepy. Sleep rejected");
    }
}

bool EnigmaIOTNodeClass::unencryptedDataMessage (const uint8_t* data, size_t len, bool controlMessage, nodePayloadEncoding_t payloadEncoding) {
    /*
    * ------------------------------------------------------------------------
    *| msgType (1) | NodeId (2) | Counter (2) | PayloadType (1) | Data (....) |
    * ------------------------------------------------------------------------
    */

    uint8_t buf[MAX_MESSAGE_LENGTH];
    uint16_t counter;
    uint16_t nodeId = node.getNodeId ();

    uint8_t nodeId_idx = 1;
    uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
    uint8_t encoding_idx = counter_idx + sizeof (int16_t);
    uint8_t data_idx = encoding_idx + sizeof (int8_t);

    uint8_t packet_length = data_idx + len;

    if (!data) {
        return false;
    }

    if (controlMessage) {
        return false; // Unencrypted control data not implemented
    } else {
        buf[0] = (uint8_t)UNENCRYPTED_NODE_DATA;
    }

    memcpy (buf + nodeId_idx, &nodeId, sizeof (uint16_t));

    if (!controlMessage) { // Control messages do not use counter
        if (useCounter) {
            counter = node.getLastMessageCounter () + 1;
            node.setLastMessageCounter (counter);
            rtcmem_data.lastMessageCounter = counter;
        } else {
            counter = (uint16_t)(Crypto.random ());
        }

        memcpy (buf + counter_idx, &counter, sizeof (uint16_t));
    }

    buf[encoding_idx] = (uint8_t)payloadEncoding;

    memcpy (buf + data_idx, data, len);

    DEBUG_INFO (" -------> UNENCRYPTED DATA");
    DEBUG_VERBOSE ("Unencrypted data message: %s", printHexBuffer (buf, packet_length));

#if DEBUG_LEVEL >= VERBOSE
    char macStr[ENIGMAIOT_ADDR_LEN * 3];
    DEBUG_DBG ("Destination address: %s", mac2str (rtcmem_data.gateway, macStr));
#endif

    if (useCounter && !otaRunning) { // RTC must not be written if OTA is running. OTA uses RTC memmory to signal 2nd firmware boot
        if (!saveRTCData ()) {
            DEBUG_ERROR ("Error saving data on RTC");
        }
    }

    return (comm->send (rtcmem_data.gateway, buf, packet_length) == 0);
}


bool EnigmaIOTNodeClass::dataMessage (const uint8_t* data, size_t len, bool controlMessage, bool encrypt, nodePayloadEncoding_t payloadEncoding) {
    /*
    * ----------------------------------------------------------------------------------------
    *| msgType (1) | IV (12) | length (2) | NodeId (2) | Counter (2) | Data (....) | tag (16) |
    * ----------------------------------------------------------------------------------------
    */

    if (!encrypt) {
        return unencryptedDataMessage (data, len, controlMessage, payloadEncoding);
    }

    uint8_t buf[MAX_MESSAGE_LENGTH];
    uint8_t tag[TAG_LENGTH];
    uint16_t counter;
    uint16_t nodeId = node.getNodeId ();

    uint8_t iv_idx = 1;
    uint8_t length_idx = iv_idx + IV_LENGTH;
    uint8_t nodeId_idx = length_idx + sizeof (int16_t);
    uint8_t counter_idx = nodeId_idx + sizeof (int16_t);
    uint8_t encoding_idx;
    uint8_t data_idx;
    if (!controlMessage) {
        encoding_idx = counter_idx + sizeof (int16_t);
        data_idx = encoding_idx + sizeof (int8_t);
    } else {
        data_idx = counter_idx + sizeof (int16_t);
    }
    uint8_t tag_idx = data_idx + len;


    if (!data) {
        return false;
    }

    if (controlMessage) {
        buf[0] = (uint8_t)CONTROL_DATA;
    } else {
        buf[0] = (uint8_t)SENSOR_DATA;
    }

    CryptModule::random (buf + iv_idx, IV_LENGTH);

    DEBUG_VERBOSE ("IV: %s", printHexBuffer (buf + iv_idx, IV_LENGTH));

    memcpy (buf + nodeId_idx, &nodeId, sizeof (uint16_t));

    if (!controlMessage) { // Control messages do not use counter
        if (useCounter) {
            counter = node.getLastMessageCounter () + 1;
            node.setLastMessageCounter (counter);
            rtcmem_data.lastMessageCounter = counter;
        } else {
            counter = (uint16_t)(Crypto.random ());
        }

        memcpy (buf + counter_idx, &counter, sizeof (uint16_t));
    }

    buf[encoding_idx] = payloadEncoding;

    memcpy (buf + data_idx, data, len);

    uint16_t packet_length = tag_idx;

    memcpy (buf + length_idx, &packet_length, sizeof (uint16_t));

    DEBUG_VERBOSE ("Data message: %s", printHexBuffer (buf, packet_length));
    DEBUG_DBG ("Encoding: 0x%02X", payloadEncoding);

    uint8_t* crypt_buf = buf + length_idx;

    size_t cryptLen = packet_length - 1 - IV_LENGTH;

    uint8_t addDataLen = 1 + IV_LENGTH;
    uint8_t aad[AAD_LENGTH + addDataLen];

    memcpy (aad, buf, addDataLen); // Copy message upto iv

    // Copy 8 last bytes from Node Key
    memcpy (aad + addDataLen, node.getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

    if (!CryptModule::encryptBuffer (crypt_buf, cryptLen, // Encrypt from length
                                     buf + iv_idx, IV_LENGTH,
                                     node.getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of node key
                                     aad, sizeof (aad), buf + tag_idx, TAG_LENGTH)) {
        DEBUG_ERROR ("Error during encryption");
        return false;
    }

    DEBUG_VERBOSE ("Encrypted data message: %s", printHexBuffer (buf, packet_length + TAG_LENGTH));

    if (controlMessage) {
        DEBUG_INFO (" -------> CONTROL MESSAGE");
} else {
        DEBUG_INFO (" -------> DATA");
    }
#if DEBUG_LEVEL >= VERBOSE
    char macStr[ENIGMAIOT_ADDR_LEN * 3];
    DEBUG_DBG ("Destination address: %s", mac2str (rtcmem_data.gateway, macStr));
#endif

    if (useCounter && !otaRunning) { // RTC must not be written if OTA is running. OTA uses RTC memmory to signal 2nd firmware boot
		if (!saveRTCData ()) {
			DEBUG_ERROR ("Error saving data on RTC");
		}
    }

    return (comm->send (rtcmem_data.gateway, buf, packet_length + TAG_LENGTH) == 0);
}

bool EnigmaIOTNodeClass::processGetSleepTimeCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
    uint8_t buffer[MAX_MESSAGE_LENGTH];
    uint8_t bufLength;

    DEBUG_DBG ("Get Sleep command received");
    DEBUG_VERBOSE ("%s", printHexBuffer (data, len));

    buffer[0] = control_message_type::SLEEP_ANS;
    // TODO get real sleep time
    uint32_t sleepTime = getSleepTime ();
    memcpy (buffer + 1, &sleepTime, sizeof (sleepTime));
    bufLength = 5;

    if (sendData (buffer, bufLength, true)) {
        DEBUG_DBG ("Sleep time is %d seconds", sleepTime);
        DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
        return true;
    } else {
        DEBUG_WARN ("Error sending version response");
        return false;
    }
}

bool EnigmaIOTNodeClass::processSetIdentifyCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
    uint8_t buffer[MAX_MESSAGE_LENGTH];
    uint8_t bufLength;

    DEBUG_DBG ("Set Identify command received");
    DEBUG_VERBOSE ("%s", printHexBuffer (data, len));

    DEBUG_WARN ("IDENTIFY");
    startIdentifying (1000);
}

bool EnigmaIOTNodeClass::processGetRSSICommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
	requestSearchGateway = true;
	requestReportRSSI = true;
}

bool EnigmaIOTNodeClass::processSetResetConfigCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
    uint8_t buffer[MAX_MESSAGE_LENGTH];
    uint8_t bufLength;

    DEBUG_DBG ("Reset Config command received");
    DEBUG_VERBOSE ("%s", printHexBuffer (data, len));

	buffer[0] = control_message_type::RESET_ANS;
	bufLength = 1;

	configCleared = true; // Disable any possible saving to flash or RTC memory

	if (sendData (buffer, bufLength, true)) {
		DEBUG_DBG ("Reset Config about to be executed", sleepTime);
		DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
	} else {
		DEBUG_WARN ("Error sending Reset Config response");
	}

	clearRTC ();
	clearFlash ();

	ESP.restart ();
}

void EnigmaIOTNodeClass::clearRTC () {
	uint8_t data[sizeof (rtcmem_data)];

	memset (data, 0, sizeof (rtcmem_data));

	ESP.rtcUserMemoryWrite (RTC_ADDRESS, (uint32_t*)data, sizeof (rtcmem_data));

	DEBUG_DBG ("RTC Cleared");
}

bool EnigmaIOTNodeClass::processSetSleepTimeCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
    uint8_t buffer[MAX_MESSAGE_LENGTH];
    uint8_t bufLength;

    DEBUG_DBG ("Set Sleep command received");
    DEBUG_VERBOSE ("%s", printHexBuffer (data, len));
    if (!SPIFFS.begin ()) {
        DEBUG_ERROR ("Error mounting flash");
    }
    bool result = loadFlashData ();
    if (!result) {
        DEBUG_WARN ("Error loading configuration");
    }

    buffer[0] = control_message_type::SLEEP_ANS;

    uint32_t sleepTime;
    memcpy (&sleepTime, data + 1, sizeof (uint32_t));
    DEBUG_DBG ("Sleep time requested: %d", sleepTime);
    setSleepTime (sleepTime);
    // TODO Store config on flash if sleep time > 0
    sleepTime = getSleepTime ();
    if (sleepTime > 0) {
        if (result) {
            if (result = saveFlashData ()) {
                DEBUG_DBG ("Saved config data after set sleep time command");
            } else {
                DEBUG_WARN ("Error saving data after set sleep time command");
            }
        }
        SPIFFS.end ();
    }
    memcpy (buffer + 1, &sleepTime, sizeof (sleepTime));
    bufLength = 5;

    if (sendData (buffer, bufLength, true)) {
        DEBUG_DBG ("Sleep time is %d seconds", sleepTime);
        DEBUG_VERBOSE ("Data: %s", printHexBuffer (buffer, bufLength));
        return result;
    } else {
        DEBUG_WARN ("Error sending version response");
        return false;
    }
}


bool EnigmaIOTNodeClass::processVersionCommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
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
    } else {
        DEBUG_WARN ("Error sending version response");
        return false;
    }
}

bool EnigmaIOTNodeClass::processOTACommand (const uint8_t* mac, const uint8_t* data, uint8_t len) {
	const uint8_t MAX_OTA_RESPONSE_LENGTH = 4;
	
	uint8_t responseBuffer[MAX_OTA_RESPONSE_LENGTH];

    //DEBUG_VERBOSE ("Data: %s", printHexBuffer (data, len));
    uint16_t msgIdx;
    static char md5buffer[33];
    char md5calc[32];
    static uint16_t numMsgs;
    static uint32_t otaSize;
    static uint16_t oldIdx;
	static bool otaRecoverRequested = false;
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
			if (!otaRecoverRequested) {
				otaRecoverRequested = true;
				responseBuffer[0] = control_message_type::OTA_ANS;
				responseBuffer[1] = ota_status::OTA_OUT_OF_SEQUENCE;
				memcpy (responseBuffer + 2, (uint8_t*)& oldIdx, sizeof (oldIdx));
				sendData (responseBuffer, 4, true);
				DEBUG_ERROR ("%u OTA messages missing before %u", msgIdx - oldIdx - 1, msgIdx);
				//otaRunning = false;
				//otaError = true;
			}
            return true;
        } else {
			oldIdx = msgIdx;
			otaRecoverRequested = false;
        }
    }
    lastOTAmsg = millis ();

    if (msgIdx == 0) {
        if (dataLen < 38) {
            DEBUG_ERROR ("OTA message #0 is too short: %u bytes", dataLen + 3);
            return false;
        }
        memcpy (&otaSize, dataPtr, sizeof (uint32_t));
        DEBUG_INFO ("OTA size: %u bytes", otaSize);
        dataPtr += sizeof (uint32_t);
        dataLen -= sizeof (uint32_t);
        memcpy (&numMsgs, dataPtr, sizeof (uint16_t));
        DEBUG_INFO ("Number of OTA messages: %u", numMsgs);
        dataPtr += sizeof (uint16_t);
        dataLen -= sizeof (uint16_t);
        memcpy (md5buffer, dataPtr, 32);
        md5buffer[32] = '\0';
        DEBUG_VERBOSE ("MD5: %s", printHexBuffer ((uint8_t*)md5buffer, 32));
        otaRunning = true;
        otaError = false;
        _md5.begin ();
        responseBuffer[0] = control_message_type::OTA_ANS;
        responseBuffer[1] = ota_status::OTA_STARTED;
        if (sendData (responseBuffer, 2, true)) {
            DEBUG_WARN ("OTA STARTED");
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
            // Process OTA Update
            size_t numBytes = Update.write (dataPtr, dataLen);
            totalBytes += dataLen;
            DEBUG_WARN ("%u bytes written. Total %u", numBytes, totalBytes);
        } else {
            if (!otaError) {
                otaError = true;
                responseBuffer[0] = control_message_type::OTA_ANS;
                responseBuffer[1] = ota_status::OTA_START_ERROR;
                sendData (responseBuffer,2, true);
                DEBUG_ERROR ("OTA error. Message 0 not received");
            }
        }
    }

    if (msgIdx == numMsgs && otaRunning) {
        StreamString otaErrorStr;

        DEBUG_INFO ("OTA end");
        _md5.calculate ();
        DEBUG_DBG ("OTA MD5 %s", _md5.toString ().c_str ());
        _md5.getChars (md5calc);
        if (!memcmp (md5calc, md5buffer, 32)) {
            responseBuffer[0] = control_message_type::OTA_ANS;
            responseBuffer[1] = ota_status::OTA_CHECK_OK;
            sendData (responseBuffer, 2, true);
			DEBUG_WARN ("OTA MD5 check OK");
		} else {
            responseBuffer[0] = control_message_type::OTA_ANS;
            responseBuffer[1] = ota_status::OTA_CHECK_FAIL;
            sendData (responseBuffer, 2, true);
            DEBUG_ERROR ("OTA MD5 check failed");
        }
        Serial.print ('.');
        while (!Update.isFinished ()) {
            Serial.print ('.');
            delay (100);
        }
        Serial.println ();

        if (Update.end ()) {
			responseBuffer[0] = control_message_type::OTA_ANS;
			responseBuffer[1] = ota_status::OTA_FINISHED;
			sendData (responseBuffer, 2, true);
			uint8_t otaErrorCode = Update.getError ();
            DEBUG_WARN ("OTA Finished OK");
            DEBUG_WARN ("OTA eror code: %d", otaErrorCode);
            //ESP.restart ();
			otaRunning = false;
			shouldRestart = true;
            return true; // Restart does not happen inmediatelly, so code goes on
        } else {
            responseBuffer[0] = control_message_type::OTA_ANS;
            responseBuffer[1] = ota_status::OTA_CHECK_FAIL;
            sendData (responseBuffer, 2, true);
            uint8_t otaErrorCode = Update.getError ();
            Update.printError (otaErrorStr);
            otaErrorStr.trim (); // remove line ending
            DEBUG_ERROR ("OTA Failed");
            DEBUG_WARN ("OTA eror code: %s", otaErrorStr.c_str ());
            Serial.println ("OTA failed");
            return false;
        }
        delay (500);
        restart ();
    }

    return true;
}

void EnigmaIOTNodeClass::restart (bool reboot) {
    rtcmem_data.nodeRegisterStatus = UNREGISTERED;
    rtcmem_data.nodeKeyValid = false; // Force resync
	if (!saveRTCData ()) {
		DEBUG_ERROR ("Error saving data on RTC");
	}
	DEBUG_WARN ("Reset configuration data in RTC memory");
    if (reboot)
        ESP.restart (); // Reboot to recover normal status
}

bool EnigmaIOTNodeClass::processControlCommand (const uint8_t* mac, const uint8_t* data, size_t len) {

	DEBUG_VERBOSE ("Data: %s", printHexBuffer (data, len));
    switch (data[0]) {
    case control_message_type::VERSION:
        return processVersionCommand (mac, data, len);
    case control_message_type::SLEEP_GET:
        return processGetSleepTimeCommand (mac, data, len);
    case control_message_type::SLEEP_SET:
        return processSetSleepTimeCommand (mac, data, len);
    case control_message_type::IDENTIFY:
        return processSetIdentifyCommand (mac, data, len);
	case control_message_type::RESET:
		return processSetResetConfigCommand (mac, data, len);
	case control_message_type::RSSI_GET:
		return processGetRSSICommand (mac, data, len);
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

bool EnigmaIOTNodeClass::processDownstreamData (const uint8_t* mac, const uint8_t* buf, size_t count, bool control) {
    /*
    * --------------------------------------------------------------------------
    *| msgType (1) | IV (12) | length (2) | NodeId (2) | Data (....) | Tag (16) |
    * --------------------------------------------------------------------------
    */

    //uint8_t msgType_idx = 0;
    uint8_t iv_idx = 1;
    uint8_t length_idx = iv_idx + IV_LENGTH;
    uint8_t nodeId_idx = length_idx + sizeof (int16_t);
    uint8_t encoding_idx;
    uint8_t data_idx;
    if (!control) {
        encoding_idx = nodeId_idx + sizeof (int16_t);
        data_idx = encoding_idx + sizeof (int8_t);
    } else {
        data_idx = nodeId_idx + sizeof (int16_t);
    }
    uint8_t tag_idx = count - TAG_LENGTH;

    uint8_t addDataLen = 1 + IV_LENGTH;
    uint8_t aad[AAD_LENGTH + addDataLen];

    memcpy (aad, buf, addDataLen); // Copy message upto iv

    // Copy 8 last bytes from NetworkKey
    memcpy (aad + addDataLen, node.getEncriptionKey () + KEY_LENGTH - AAD_LENGTH, AAD_LENGTH);

    uint8_t packetLen = count - TAG_LENGTH;

    if (!CryptModule::decryptBuffer (buf + length_idx, packetLen - 1 - IV_LENGTH, // Decrypt from nodeId
                                     buf + iv_idx, IV_LENGTH,
                                     node.getEncriptionKey (), KEY_LENGTH - AAD_LENGTH, // Use first 24 bytes of network key
                                     aad, sizeof (aad), buf + tag_idx, TAG_LENGTH)) {
        DEBUG_ERROR ("Error during decryption");
        return false;
    }

    DEBUG_VERBOSE ("Decripted downstream message: %s", printHexBuffer (buf, count - TAG_LENGTH));

    if (control) {
        DEBUG_INFO ("Control command");
        DEBUG_VERBOSE ("Data: %s", printHexBuffer (&buf[data_idx], tag_idx - data_idx));
        return processControlCommand (mac, &buf[data_idx], tag_idx - data_idx);
    }

    DEBUG_VERBOSE ("Sending data notification. Payload length: %d", tag_idx - data_idx);
    if (notifyData) {
        notifyData (mac, &buf[data_idx], tag_idx - data_idx, (nodeMessageType_t)(buf[0]), (nodePayloadEncoding_t)(buf[encoding_idx]));
    }

    return true;

}


nodeInvalidateReason_t EnigmaIOTNodeClass::processInvalidateKey (const uint8_t* mac, const uint8_t* buf, size_t count) {
#define IKMSG_LEN 2
    if (buf && count < IKMSG_LEN) {
        return UNKNOWN_ERROR;
    }

    DEBUG_WARN ("Invalidate key request. Reason: %u", buf[1]);
    uint8_t reason = buf[1];
    if (reason < KEY_EXPIRED) {
        if (dataMessageSentLength > 0)
            dataMessageSendPending = true; // Start last data retransmission
        // TODO: CHeck what happens in initial server hello. This should be enough
    }

    return (nodeInvalidateReason_t)reason;
}

void EnigmaIOTNodeClass::manageMessage (const uint8_t* mac, const uint8_t* buf, uint8_t count) {
    DEBUG_INFO ("Reveived message. Origin MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DEBUG_VERBOSE ("Received data: %s", printHexBuffer (const_cast<uint8_t*>(buf), count));
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
				// mark node as registered
				node.setKeyValid (true);
				rtcmem_data.nodeKeyValid = true;
				node.setKeyValidFrom (millis ());
				node.setLastMessageCounter (0);
				node.setStatus (REGISTERED);
				rtcmem_data.nodeRegisterStatus = REGISTERED;

				// save context to RTC memory
				memcpy (rtcmem_data.nodeKey, node.getEncriptionKey (), KEY_LENGTH);
				rtcmem_data.lastMessageCounter = 0;
				rtcmem_data.nodeId = node.getNodeId ();
				
				if (!saveRTCData ()) {
					DEBUG_ERROR ("Error saving data on RTC");
				}

				// request clock sync if non sleepy
				//if (!node.getSleepy () && node.isRegistered ())
				//	clockRequest ();

#if DEBUG_LEVEL >= INFO
				node.printToSerial (&DEBUG_ESP_PORT);
#endif

				// send notification to user code
				if (notifyConnection) {
					notifyConnection ();
				}
				// Resend last message in case of it is still pending to be sent.
				// If key expired it was successfully sent before so retransmission is not needed 
				if (invalidateReason < KEY_EXPIRED && dataMessageSentLength > 0) {
					if (node.getStatus () == REGISTERED && node.isKeyValid ()) {
						if (dataMessageSendPending && dataMessageSentLength > 0) {
							DEBUG_INFO ("Data pending to be sent. Length: %u", dataMessageSentLength);
							DEBUG_VERBOSE ("Data sent: %s", printHexBuffer (dataMessageSent, dataMessageSentLength));
                            dataMessage ((uint8_t*)dataMessageSent, dataMessageSentLength, false, dataMessageEncrypt, dataMessageSendEncoding);
                            //dataMessageSentLength = 0;
                            dataMessageSendPending = false;
                            
							flashBlue = true;
						}
					}
				}


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
		requestSearchGateway = true;
		node.reset ();
		TimeManager.reset ();
		timeSyncPeriod = QUICK_SYNC_TIME;
        if (notifyDisconnection) {
            notifyDisconnection (invalidateReason);
        }
        break;
    case DOWNSTREAM_DATA_SET:
        DEBUG_INFO (" <------- DOWNSTREAM DATA SET");
        if (processDownstreamData (mac, buf, count)) {
            DEBUG_INFO ("Downstream Data set OK");
        }
        break;
	case DOWNSTREAM_DATA_GET:
		DEBUG_INFO (" <------- DOWNSTREAM DATA GET");
		if (processDownstreamData (mac, buf, count)) {
			DEBUG_INFO ("Downstream Data set OK");
		}
		break;
    case DOWNSTREAM_CTRL_DATA:
        DEBUG_INFO (" <------- DOWNSTREAM CONTROL DATA");
        if (processDownstreamData (mac, buf, count, true)) {
            DEBUG_INFO ("Downstream Data OK");
        }
        break;
	case CLOCK_RESPONSE:
		DEBUG_INFO (" <------- CLOCK RESPONSE");
		if (clockSyncEnabled) {
			if (processClockResponse (mac, buf, count)) {
				DEBUG_INFO ("Clock Response OK");
			}
		}
		break;
	}
}


void EnigmaIOTNodeClass::getStatus (uint8_t* mac_addr, uint8_t status) {
	gatewaySearchStarted = false;
	if (status == 0) {
        DEBUG_DBG ("SENDStatus OK");
		rtcmem_data.commErrors = 0;
    } else {
		rtcmem_data.commErrors++;
		if (!saveRTCData ()) {
			DEBUG_ERROR ("Error saving data on RTC");
		}
        DEBUG_ERROR ("SENDStatus ERROR %d. Comm errors %u", status, rtcmem_data.commErrors);
    }
}


EnigmaIOTNodeClass EnigmaIOTNode;

#endif