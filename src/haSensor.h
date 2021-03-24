#ifndef _HA_SENSOR_h
#define _HA_SENSOR_h

#include "haEntity.h"

class HASensor : public HAEntity {
   //friend class HAEntity;
// protected:
    //char* payloadOff;
    //char* payloadOn;
    //haDeviceClass_t deviceClass = none;

public:
    HASensor () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = SENSOR;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    void setDeviceClass (haSensorClass_t devClass);
    
    void setExpireTime (uint payload);

    void setUnitOfMeasurement (const char* payload);
    
    void setValueField (const char* payload);

    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, uint8_t* msgPack, size_t len);

    static String deviceClassStr (haSensorClass_t sensorClass);

};

#endif // _HA_COVER_h