#ifndef _HA_TRIGGER_h
#define _HA_TRIGGER_h

#include "haEntity.h"

#if SUPPORT_HA_DISCOVERY

typedef enum {
    button_short_press     = 0,
    button_short_release   = 1,
    button_long_press      = 2, 
    button_long_release    = 3,
    button_double_press    = 4,
    button_triple_press    = 5,
    button_quadruple_press = 6,
    button_quintuple_press = 7
} ha_triggerType_t;

static const char* ha_triggerTypeStr[] = {
    "button_short_press",
    "button_short_release",
    "button_long_press",
    "button_long_release",
    "button_double_press",
    "button_triple_press",
    "button_quadruple_press",
    "button_quintuple_press"
};

typedef enum {
    turn_on  = 0,
    turn_off = 1,
    button_1 = 2,
    button_2 = 3,
    button_3 = 4,
    button_4 = 5,
    button_5 = 6,
    button_6 = 7
} ha_triggerSubtype_t;

static const char* ha_triggerSubtypeStr[] = {
    "turn_on",
    "turn_off",
    "button_1",
    "button_2",
    "button_3",
    "button_4",
    "button_5",
    "button_6"
};

class HATrigger : public HAEntity {
   //friend class HAEntity;
// protected:
    //char* payloadOff;
    //char* payloadOn;
    //haDeviceClass_t deviceClass = none;

public:
    HATrigger () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = DEVICE_TRIGGER;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    static const char* getTriggerTypeStr (int type) {
        if (type >= button_short_press || type <= button_quintuple_press) {
            return ha_triggerTypeStr[type];
        } else {
            return NULL;
        }
    }

    static const char* getTriggerSubtypeStr (int subtype) {
        if (subtype >= turn_on || subtype <= button_6) {
            return ha_triggerSubtypeStr[subtype];
        } else {
            return NULL;
        }
    }

    void setPayload (const char* payload);

    void setType (ha_triggerType_t type) {
        (*entityConfig)[ha_type] = type;
    }
    
    void setType (const char* type) {
        (*entityConfig)[ha_type] = type;
    }

    void setSubtype (ha_triggerSubtype_t subtype) {
        (*entityConfig)[ha_subtype] = subtype;
    }

    void setSubtype (const char* subtype) {
        (*entityConfig)[ha_subtype] = subtype;
    }

    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON);

};

#endif // SUPPORT_HA_DISCOVERY

#endif // _HA_TRIGGER_h