/**
  * @file haTrigger.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief Home Assistant trigger integration
  */


#ifndef _HA_TRIGGER_h
#define _HA_TRIGGER_h

#include "haEntity.h"

#if SUPPORT_HA_DISCOVERY

/**
 * The mqtt device trigger platform uses an MQTT message payload to generate device trigger events. An MQTT device trigger is a better option than a binary sensor
 *   for buttons, remote controls etc.
 *
 *   MQTT device triggers are only supported through MQTT discovery, manual setup through configuration.yaml is not supported.
 *
 * https://www.home-assistant.io/integrations/device_trigger.mqtt/
 *
 * An example of trigger discovery message may be like this:
 *
 * Topic:
 *    homeassistant/device-trigger/button_button_short_press_button_1/config
 *
 * Payload:
 *
 * {
 *    "automation_type":"trigger",      // string REQUIRED The type of automation, must be ‘trigger’
 *    "topic":"EnigmaTest/button/data", // string REQUIRED The MQTT topic subscribed to receive trigger events
 *    "payload":"{\"button\":1}",       // string (optional) Optional payload to match the payload being sent over the topic
 *    "type":"button_short_press",      // string REQUIRED The type of the trigger, e.g. button_short_press.
 *                                      //   Entries supported by the frontend: button_short_press, button_short_release, button_long_press, button_long_release,
 *                                      //   button_double_press, button_triple_press, button_quadruple_press, button_quintuple_press.
 *                                      //   If set to an unsupported value, will render as subtype type, e.g. First button spammed with type set to spammed and subtype set to button_1
 *    "subtype":"button_1",             // string REQUIRED The subtype of the trigger, e.g. button_1. 
 *                                      //   Entries supported by the frontend: turn_on, turn_off, button_1, button_2, button_3, button_4, button_5, button_6.
 *                                      //   If set to an unsupported value, will render as subtype type, e.g. left_button pressed with type set to button_short_press and subtype set to left_button
 *    "device":{                        // Information about the device this device trigger is a part of to tie it into the device registry
 *        "name":"button",              // string (optional) The name of the device
 *        "identifiers":"button"        // list | string (optional) A list of IDs that uniquely identify the device. For example a serial number
 *    }
 * }
 *
 * Template message for switch is this
 *
 *  Topic:
 *    homeassistant/device-trigger/<node_name>_<type>_<subtype>/config
 *
 * Payload
 *
 * ```
 *  {
 *     "automation_type":"trigger",
 *     "topic":"<network_name>/<node_name>/data",
 *     "payload":<payload_value>,
 *     "type":<type>,
 *     "subtype":<subtype>,
 *     "device":{
 *         "name":<node_name>_<type>_<subtype>,
 *         "identifiers":<node_name>_<type>_<subtype>
 *     }
 * }
 * ```
 *
 * Message to gateway is like following
 *
 * ```
 * {
 *    "pl":<payload_value>,
 *    "type":<type>,
 *    "stype":<subtype>
 * }
 * ```
 */


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

public:
    /**
     * @brief Trigger constructor
     */
    HATrigger () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = DEVICE_TRIGGER;
        (*entityConfig)[ha_device_type] = deviceType;
    };


    /**
     * @brief Returns string that correspond with trigger type in ha_triggerType_t format
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#type
     * @param type Trigger type
     */
    static const char* getTriggerTypeStr (int type) {
        if (type >= button_short_press || type <= button_quintuple_press) {
            return ha_triggerTypeStr[type];
        } else {
            return NULL;
        }
    }

    /**
     * @brief Returns string that correspond with trigger subtype in ha_triggerSubtype_t format
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#subtype
     * @param subtype Trigger type
     */
    static const char* getTriggerSubtypeStr (int subtype) {
        if (subtype >= turn_on || subtype <= button_6) {
            return ha_triggerSubtypeStr[subtype];
        } else {
            return NULL;
        }
    }

    /**
     * @brief Optional payload to match the payload being sent over the topic
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#payload
     * @param payload Payload string
     */
    void setPayload (const char* payload);

    /**
     * @brief Set trigger type as ha_triggerType_t
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#type
     * @param type Payload string
     */
    void setType (ha_triggerType_t type) {
        (*entityConfig)[ha_type] = type;
    }
    
    /**
     * @brief Set trigger type as char string
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#type
     * @param type Payload string
     */
    void setType (const char* type) {
        (*entityConfig)[ha_type] = type;
    }

    /**
     * @brief Set trigger subtype as ha_triggerSubtype_t
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#subtype
     * @param subtype Payload string
     */
    void setSubtype (ha_triggerSubtype_t subtype) {
        (*entityConfig)[ha_subtype] = subtype;
    }

    /**
     * @brief Set trigger subtype as char string
     *          https://www.home-assistant.io/integrations/device_trigger.mqtt/#subtype
     * @param subtype Payload string
     */
    void setSubtype (const char* subtype) {
        (*entityConfig)[ha_subtype] = subtype;
    }

    /**
     * @brief Allows Gateway to get Home Assistant discovery message using Trigger template
     * @param buffer Buffer to hold message string
     * @param buflen Buffer size
     * @param nodeName Originating node name
     * @param networkName EnigmaIOT network name
     * @param inputJSON JSON object sent by node with needed data to fill template in
     * @return Discovery message payload
     */
    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON);

};

#endif // SUPPORT_HA_DISCOVERY

#endif // _HA_TRIGGER_h