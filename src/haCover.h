#ifndef _HA_COVER_h
#define _HA_COVER_h

#include "haEntity.h"

class HACover : public HAEntity {
   //friend class HAEntity;
// protected:
    //char* payloadOff;
    //char* payloadOn;
    //haDeviceClass_t deviceClass = none;

public:
    HACover () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = COVER;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    void setDeviceClass (haCoverClass_t devClass);
    
    void setPayloadOpen (const char* payload);

    void setPayloadClose (const char* payload);

    void setPayloadStop (const char* payload);

    void setPayloadGoto (const char* payload);

    void setStateOpen (const char* payload);
    
    void setStateOpening (const char* payload);
    
    void setStateClosed (const char* payload);
    
    void setStateClosing (const char* payload);
    
    void setStateStopped (const char* payload);

    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, uint8_t* msgPack, size_t len);

    static String deviceClassStr (haCoverClass_t sensorClass);

};

#endif // _HA_COVER_h