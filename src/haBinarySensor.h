#ifndef _BINARY_SENSOR_h
#define _BINARY_SENSOR_h

#include "haEntity.h"

class HABinarySensor : public HAEntity {
   //friend class HAEntity;
// protected:
    //char* payloadOff;
    //char* payloadOn;
    //haDeviceClass_t deviceClass = none;

public:
    HABinarySensor () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = BINARY_SENSOR;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    void setDeviceClass (haBinarySensorClass_t devClass);
    
    void setPayloadOn (const char* payload);

    void setPayloadOff (const char* payload);

    void setPayloadOn (int payload);

    void setPayloadOff (int payload);

    void setValueField (const char* payload);

    void setOffDelay (uint payload);

    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, uint8_t* msgPack, size_t len);

    static String deviceClassStr (haBinarySensorClass_t sensorClass);

    void addExpiration (uint seconds) {
        if (seconds > 0) {
            (*entityConfig)[ha_expiration] = seconds;
        }
    }


};

#endif // _BINARY_SENSOR_h