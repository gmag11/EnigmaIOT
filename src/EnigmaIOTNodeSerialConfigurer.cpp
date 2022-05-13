#include "EnigmaIOTNodeSerialConfigurer.h"

#include "EnigmaIOTdebug.h"
#include "cryptModule.h"
#include "helperFunctions.h"

bool askInput(const String& prompt, const String& invalidMessage, String& result) {
    while (true) {
        yield();
        Serial.println(prompt);
        result = Serial.readStringUntil('\n');
        result.trim();
        if (result.length() < 1) {
            Serial.println(invalidMessage);
            continue;
        }
        return true;
    }
}

bool EnigmaIOTNodeSerialConfigurerClass::configure(rtcmem_data_t* data) {
    time_t origSeiralTimeout = Serial.getTimeout();

    Serial.setTimeout(10000);

    data->lastMessageCounter = 0;
    data->nodeRegisterStatus = UNREGISTERED;
    data->nodeKeyValid = false;

    bool allInputsValid = true;

    String ssid;
    if (askInput("Enter EnigmaIOT ssid:", "Empty ssid, try again", ssid)) {
        strncpy(data->networkName, ssid.c_str(), NETWORK_NAME_LENGTH - 1);
    } else {
        allInputsValid = false;
    }

    String password;
    if (askInput("Enter EnigmaIOT ssid password:", "Empty password, try again", password)) {
        strncpy((char*)(data->networkKey), password.c_str(), KEY_LENGTH);
        CryptModule::getSHA256(data->networkKey, KEY_LENGTH);
        DEBUG_DBG("Calculated network key: %s", printHexBuffer(data->networkKey, KEY_LENGTH));
    } else {
        allInputsValid = false;
    }

    String nodeName;
    if (askInput("Enter EnigmaIOT node name:", "Empty node name, try again", nodeName)) {
        strncpy(data->nodeName, nodeName.c_str(), NODE_NAME_LENGTH);
    } else {
        allInputsValid = false;
    }

    String sleepTime;
    if (askInput("Enter EnigmaIOT sleep time:", "Empty sleep time, try again", sleepTime)) {
        int sleepyVal = atoi(sleepTime.c_str());
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
    Serial.setTimeout(origSeiralTimeout);

    return allInputsValid;
}

EnigmaIOTNodeSerialConfigurerClass EnigmaIOTNodeSerialConfigurer;