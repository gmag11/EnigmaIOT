
#include "EnigmaIOTNodeWifiConfigurer.h"

#include "EnigmaIOTdebug.h"
#include "cryptModule.h"
#include "helperFunctions.h"
#if defined ESP32
#include "esp_wifi.h"
#endif

bool EnigmaIOTNodeWifiConfigurerClass::configure(rtcmem_data_t* data) {
    AsyncWebServer server(80);
    DNSServer dns;

    bool regexResult = true;

    char sleepy[5] = "10";
    char nodeName[NODE_NAME_LENGTH] = "";

    wifiManager = new AsyncWiFiManager(&server, &dns);
#if DEBUG_LEVEL == NONE
    wifiManager->setDebugOutput(false);
#endif

    AsyncWiFiManagerParameter sleepyParam("sleepy", "Sleep Time", sleepy, 5, "required type=\"number\" min=\"0\" max=\"13600\" step=\"1\"");
    AsyncWiFiManagerParameter nodeNameParam("nodename", "Node Name", nodeName, NODE_NAME_LENGTH, "type=\"text\" pattern=\"^[^/\\\\]+$\" maxlength=32");

    wifiManager->setCustomHeadElement("<style>input:invalid {border: 2px dashed red;input:valid{border: 2px solid black;}</style>");
    wifiManager->addParameter(&sleepyParam);
    wifiManager->addParameter(&nodeNameParam);

    if (notifyWiFiManagerStarted) {
        notifyWiFiManagerStarted();
    }
    wifiManager->setConnectTimeout(30);
    wifiManager->setBreakAfterConfig(true);
    wifiManager->setTryConnectDuringConfigPortal(false);
    char apname[64];
#ifdef ESP8266
    snprintf(apname, 64, "EnigmaIoTNode%06x", ESP.getChipId());
#elif defined ESP32
    snprintf(apname, 64, "EnigmaIoTNode%06x", (uint32_t)(ESP.getEfuseMac() & (uint64_t)0x0000000000FFFFFF));
#endif
    DEBUG_VERBOSE("Start AP: %s", apname);

    boolean result = wifiManager->startConfigPortal(apname, NULL);
    if (result) {
        DEBUG_DBG("==== Config Portal result ====");

        DEBUG_DBG("Network Name: %s", WiFi.SSID().c_str());
#ifdef ESP8266
        station_config wifiConfig;
        if (!wifi_station_get_config(&wifiConfig)) {
            DEBUG_WARN("Error getting WiFi config");
        }
        DEBUG_DBG("WiFi password: %s", wifiConfig.password);
        const char* netkey = (char*)(wifiConfig.password);
#elif defined ESP32
        wifi_config_t wifiConfig;
        if (esp_wifi_get_config(WIFI_IF_STA, &wifiConfig)) {
            DEBUG_WARN("Error getting WiFi config");
        }
        DEBUG_WARN("WiFi password: %.*s", 64, wifiConfig.sta.password);
        const char* netkey = (char*)(wifiConfig.sta.password);
#endif
        DEBUG_DBG("Network Key: %s", netkey);
        DEBUG_DBG("Sleppy time: %s", sleepyParam.getValue());
        DEBUG_DBG("Node Name: %s", nodeNameParam.getValue());

        data->lastMessageCounter = 0;

        strncpy((char*)(data->networkKey), netkey, KEY_LENGTH);
        DEBUG_DBG("Stored network key before hash: %.*s", KEY_LENGTH, (char*)(data->networkKey));

        CryptModule::getSHA256(data->networkKey, KEY_LENGTH);
        DEBUG_DBG("Calculated network key: %s", printHexBuffer(data->networkKey, KEY_LENGTH));
        data->nodeRegisterStatus = UNREGISTERED;

        DEBUG_DBG("Temp network name: %s", WiFi.SSID().c_str());
        strncpy(data->networkName, WiFi.SSID().c_str(), NETWORK_NAME_LENGTH - 1);
        DEBUG_DBG("Stored network name: %s", data->networkName);

#ifdef ESP32
        std::regex sleepTimeRegex("(\\d)+");
        regexResult = std::regex_match(sleepyParam.getValue(), sleepTimeRegex);
        if (regexResult) {
#endif
            DEBUG_DBG("Sleep time check ok");
            int sleepyVal = atoi(sleepyParam.getValue());
            if (sleepyVal > 0) {
                data->sleepy = true;
            }
            data->sleepTime = sleepyVal;
#ifdef ESP32
        } else {
            DEBUG_WARN("Sleep time parameter error");
            result = false;
        }
#endif

#ifdef ESP32
        std::regex nodeNameRegex("^[^/\\\\]+$");
        regexResult = std::regex_match(nodeNameParam.getValue(), nodeNameRegex);
#elif defined ESP8266
        if (strstr(nodeNameParam.getValue(), "/")) {
            regexResult = false;
            DEBUG_WARN("Node name parameter error. Contains '/'");
        } else if (strstr(nodeNameParam.getValue(), "\\")) {
            regexResult = false;
            DEBUG_WARN("Node name parameter error. Contains '\\'");
        }
#endif
        if (regexResult) {
            strncpy(data->nodeName, nodeNameParam.getValue(), NODE_NAME_LENGTH);
            DEBUG_DBG("Node name: %s", data->nodeName);
        } else {
            DEBUG_WARN("Node name parameter error");
            result = false;
        }

        data->nodeKeyValid = false;
        data->crc32 = calculateCRC32((uint8_t*)(data->nodeKey), sizeof(rtcmem_data_t) - sizeof(uint32_t));
    }

    if (notifyWiFiManagerExit) {
        notifyWiFiManagerExit(result);
    }

    delete (wifiManager);

    return result;
}

EnigmaIOTNodeWifiConfigurerClass EnigmaIOTNodeWifiConfigurer;
