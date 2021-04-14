#ifndef _BINARY_SENSOR_h
#define _BINARY_SENSOR_h

#include "haEntity.h"

#if SUPPORT_HA_DISCOVERY


/**
 * The mqtt binary sensor platform uses an MQTT message received to set the binary sensor’s state to on or off.
 * The state will be updated only after a new message is published on state_topic matching payload_on or payload_off.
 * If these messages are published with the retain flag set, the binary sensor will receive an instant state update
 * after subscription and Home Assistant will display the correct state on startup. Otherwise, the initial state
 * displayed in Home Assistant will be unknown.
 *
 * Stateless devices such as buttons, remote controls etc are better represented by MQTT device triggers than by binary sensors.
 *
 * https://www.home-assistant.io/integrations/binary_sensor.mqtt/
 *
 * An example of binary sensor discovery message may be like this:
 *
 * Topic:
 *    homeassistant/binary_sensor/thermostat/config
 *
 * Payload:
 *
 * {
 *   "name":"thermostat",   // string (optional, default: MQTT Binary Sensor) The name of the binary sensor.
 *   "unique_id":"thermostat",   // string (optional) An ID that uniquely identifies this sensor.
 *                               // If two sensors have the same unique ID, Home Assistant will raise an exception.
 *   "state_topic":"EnigmaIOT/thermostat/data",    // string REQUIRED. The MQTT topic subscribed to receive sensor’s state.
 *   "payload_on":"ON",  // string (optional, default: ON) The string that represents the on state.
 *                       // It will be compared to the message in the state_topic (see value_template for details)
 *   "payload_off":"OFF",  // string (optional, default: OFF) The string that represents the off state.
 *                         // It will be compared to the message in the state_topic (see value_template for details)
 *   "value_template":"{{value_json.activation}}",  // Defines a template that returns a string to be compared to payload_on/payload_off
 *                                                  // or an empty string, in which case the MQTT message will be removed.
 *                                                  // Available variables: entity_id.
 *                                                  // Remove this option when ‘payload_on’ and ‘payload_off’ are sufficient to match
 *                                                  // your payloads (i.e no pre-processing of original message is required).
 *   "expire_after":30,  // Defines the number of seconds after the sensor’s state expires, if it’s not updated.
 *                       // After expiry, the sensor’s state becomes unavailable.
 *   "device_class":"heat"  // Sets the class of the device, changing the device state and icon that is displayed on the frontend.
 * }
 * 
 * Template message for binary sensor is this
 *
 *  Topic:
 *    homeassistant/binary_sensor/<node_name>/config
 *
 * Payload
 * {
 *   "name":<node_name>_<name_suffix>,
 *   "unique_id":<node_name>_<name_suffix>,
 *   "device_class":<device_class>,
 *   "state_topic":"<network_name>/<node_name>/data",
 *   "payload_on":<cmd_payload_on>,
 *   "payload_off":<cmd_payload_off>,
 *   "value_template":"{{value_json.<value_field>}}",
 *   "expire_after":<expire_time>
 * }
 *
 * Message template to gateway is like this
 *
 * {
 *   "pl_on":<cmd_payload_on>,
 *   "pl_off":<cmd_payload_off>,
 *   "val":<value_field>,
 *   "exp_aft":<expire_time>,
 *   "dev_cla":<device_class>,
 *   "nmsfx":<name_suffix>
 * }
 *
 *  If any of the optional values (like "pl_on" or "pl_off") is not set its key will not be sent into message
 */

class HABinarySensor : public HAEntity {
public:
    /**
     * @brief Binary sensor constructor
     */
    HABinarySensor () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = BINARY_SENSOR;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    /**
     * @brief Define binary sensor class as `haBinarySensorClass_t`
     * @param devClass Device class
     */
    void setDeviceClass (haBinarySensorClass_t devClass);
    
    /**
     * @brief Defines the string that represents the **on state**. It will be compared to the message in the **state_topic** (see value_template for details)
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#payload_on
     * @param payload ON state string
     */
    void setPayloadOn (const char* payload);

    /**
     * @brief Defines the string that represents the **off state**. It will be compared to the message in the **state_topic** (see value_template for details)
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#payload_off
     * @param payload OFF state string
     */
    void setPayloadOff (const char* payload);

    /**
     * @brief Defines a number that represents the **on state**. It will be compared to the message in the **state_topic** (see value_template for details)
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#payload_on
     * @param payload ON state value
     */
    void setPayloadOn (int payload);

    /**
     * @brief Defines a number that represents the **off state**. It will be compared to the message in the **state_topic** (see value_template for details)
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#payload_off
     * @param payload OFF state value
     */
    void setPayloadOff (int payload);

    /**
     * @brief Defines a number that represents the **on state**. It will be compared to the message in the **state_topic** (see value_template for details)
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#payload_on
     * @param payload ON state value
     */
    void setValueField (const char* payload);

    /**
     * @brief For sensors that only send on state updates (like PIRs), this sets a delay in seconds after which the sensor’s state will be updated back to off.
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#off_delay
     * @param payload Off delay in seconds
     */
    void setOffDelay (uint payload);

   
    /**
     * @brief Allows Gateway to get Home Assistant discovery message using Binary Sensor template
     * @param buffer Buffer to hold message string
     * @param buflen Buffer size
     * @param nodeName Originating node name
     * @param networkName EnigmaIOT network name
     * @param inputJSON JSON object sent by node with needed data to fill template in
     * @return Discovery message payload
     */
    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON);

    /**
     * @brief Gets binary sensor class name from `haBinarySensorClass_t`
     *          https://www.home-assistant.io/integrations/binary_sensor/#device-class
     * @param sensorClass Binary sensor class code
     */
    static String deviceClassStr (haBinarySensorClass_t sensorClass);

    /**
     * @brief Defines the number of seconds after the sensor’s state expires, if it’s not updated. After expiry, the sensor’s state becomes unavailable.
     *          https://www.home-assistant.io/integrations/binary_sensor.mqtt/#expire_after
     * @param seconds Expiration time in seconds
     */
    void addExpiration (uint seconds) {
        if (seconds > 0) {
            (*entityConfig)[ha_expiration] = seconds;
        }
    }


};
#endif // SUPPORT_HA_DISCOVERY

#endif // _BINARY_SENSOR_h