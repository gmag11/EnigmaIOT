#include "EnigmaIOTNodeJsonConfigurer.h"

#include <ArduinoJson.h>
#include <FS.h>

#include "EnigmaIOTdebug.h"
#include "cryptModule.h"
#include "helperFunctions.h"

bool EnigmaIOTNodeJsonConfigurerClass::configure(rtcmem_data_t* data) {
    File file = FILESYSTEM.open(JSON_CONFIGURER_FILE_NAME, "r");
    if (!file) {
        DEBUG_ERROR("Failed to open file %s", JSON_CONFIGURER_FILE_NAME);
        return false;
    }

    const size_t capacity = JSON_OBJECT_SIZE(8);
    DynamicJsonDocument json(capacity);
    DeserializationError configJsonDeSerError = deserializeJson(json, file);
    if (configJsonDeSerError) {
        DEBUG_ERROR("Failed to read file: /config.json, error: %s", configJsonDeSerError.c_str());
        file.close();
        return false;
    }

    data->lastMessageCounter = 0;
    data->nodeRegisterStatus = UNREGISTERED;
    data->nodeKeyValid = false;

    bool allInputsValid = true;

    DEBUG_DBG("Reading file %s", JSON_CONFIGURER_FILE_NAME);
    if (json.containsKey(F("ssid"))) {
        strncpy(data->networkName, json[F("ssid")].as<const char*>(), NETWORK_NAME_LENGTH - 1);
        DEBUG_DBG("ssid: '%s'", data->networkName);
    } else {
        DEBUG_ERROR("missing field 'ssid' in json");
        allInputsValid = false;
    }

    if (json.containsKey(F("ssidKey"))) {
        strncpy((char*)(data->networkKey), json[F("ssidKey")].as<const char*>(), KEY_LENGTH);
        DEBUG_DBG("ssidKey: '%s'", data->networkKey);
        CryptModule::getSHA256(data->networkKey, KEY_LENGTH);
        DEBUG_DBG("Calculated network key: %s", printHexBuffer(data->networkKey, KEY_LENGTH));
    } else {
        DEBUG_ERROR("missing field 'ssidKey' in json");
        allInputsValid = false;
    }

    if (json.containsKey(F("name"))) {
        strncpy(data->nodeName, json[F("name")].as<const char*>(), NODE_NAME_LENGTH);
        DEBUG_DBG("Node name: '%s'", data->nodeName);
    } else {
        DEBUG_ERROR("missing field 'name' in json");
        allInputsValid = false;
    }

    if (json.containsKey(F("sleepTime"))) {
        int sleepyVal = json[F("sleepTime")].as<int>();
        DEBUG_DBG("sleepyVal: %d", sleepyVal);
        if (sleepyVal > 0) {
            data->sleepy = true;
        }
        data->sleepTime = sleepyVal;
    } else {
        DEBUG_ERROR("missing field 'sleepTime' in json");
        allInputsValid = false;
    }

    if (allInputsValid) {
        data->crc32 = calculateCRC32((uint8_t*)(data->nodeKey), sizeof(rtcmem_data_t) - sizeof(uint32_t));
    } else {
        DEBUG_ERROR("some inputs in json file are invalid, file %s", JSON_CONFIGURER_FILE_NAME);
    }

    file.close();

    return allInputsValid;
}

EnigmaIOTNodeJsonConfigurerClass EnigmaIOTNodeJsonConfigurer;