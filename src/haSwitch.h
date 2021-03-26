#ifndef _SWITCH_h
#define _SWITCH_h

#include "haEntity.h"

class HASwitch : public HAEntity {
   //friend class HAEntity;
// protected:
    //char* payloadOff;
    //char* payloadOn;
    //haDeviceClass_t deviceClass = none;

public:
    HASwitch () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = SWITCH;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    void setPayloadOn (const char* payload);

    void setPayloadOff (const char* payload);

    void setPayloadOn (int payload);

    void setPayloadOff (int payload);

    void setStateOn (const char* payload);

    void setStateOff (const char* payload);

    void setStateOn (int payload);

    void setStateOff (int payload);

    void setValueField (const char* payload);

    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON);

};

#endif // _SWITCH_h