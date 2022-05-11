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

    data->lastMessageCounter = 0;
    data->nodeRegisterStatus = UNREGISTERED;
    data->nodeKeyValid = false;

    bool allInputsValid = true;

    if (json.containsKey(F("ssid"))) {
        strncpy(data->networkName, json[F("ssid")].as<char*>(), NETWORK_NAME_LENGTH - 1);
    } else {
        allInputsValid = false;
    }

    if (json.containsKey(F("password"))) {
        strncpy((char*)(data->networkKey), json[F("password")].as<char*>(), KEY_LENGTH);
        CryptModule::getSHA256(data->networkKey, KEY_LENGTH);
        DEBUG_DBG("Calculated network key: %s", printHexBuffer(data->networkKey, KEY_LENGTH));
    } else {
        allInputsValid = false;
    }

    if (json.containsKey(F("name"))) {
        strncpy(data->nodeName, json[F("name")].as<char*>(), NODE_NAME_LENGTH);
    } else {
        allInputsValid = false;
    }

    if (json.containsKey(F("sleepTime"))) {
        int sleepyVal = atoi(json[F("sleepTime")].as<char*>());
        if (sleepyVal > 0) {
            data->sleepy = true;
        }
        data->sleepTime = sleepyVal;
    } else {
        allInputsValid = false;
    }

    if (allInputsValid) {
        data->crc32 = calculateCRC32((uint8_t*)(data->nodeKey), sizeof(rtcmem_data_t) - sizeof(uint32_t));
    }

    return false;
}

EnigmaIOTNodeJsonConfigurerClass EnigmaIOTNodeJsonConfigurer;