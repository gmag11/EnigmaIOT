#ifndef _HA_SENSOR_h
#define _HA_SENSOR_h

#include "haEntity.h"

#if SUPPORT_HA_DISCOVERY

/**
 * This mqtt sensor platform uses the MQTT message payload as the sensor value. If messages in this state_topic are published with RETAIN flag,
 * the sensor will receive an instant update with last known value. Otherwise, the initial state will be undefined.
 *
 * https://www.home-assistant.io/integrations/sensor.mqtt/
 *
 * An example of sensor discovery message may be like this:
 *
 * Topic:
 *    homeassistant/sensor/mains_power/config
 *
 * Payload:
 *
 * {
 *    "name":"mains_power",      // string (optional, default: MQTT Sensor). The name of the MQTT sensor
 *    "unique_id":"mains_power", // string (optional) An ID that uniquely identifies this sensor. If two sensors have the same unique ID,
 *                               //    Home Assistant will raise an exception
 *    "device_class":"power",    // device_class (optional, default: None). The type/class of the sensor to set the icon in the frontend
 *                               //   https://www.home-assistant.io/integrations/sensor/#device-class
 *    "expire_after":300,        // integer (optional, default: 0) Defines the number of seconds after the sensor’s state expires,
 *                               //   if it’s not updated. After expiry, the sensor’s state becomes unavailable
 *    "json_attributes_template":"{{value_json | tojson}}", // template (optional) Defines a template to extract the JSON dictionary
 *                                                          //   from messages received on the json_attributes_topic
 *    "json_attributes_topic":"EnigmaIOT/mains/data",       // string (optional) The MQTT topic subscribed to receive a JSON dictionary
 *                                                          //   payload and then set as sensor attributes. Implies force_update of the
 *                                                          //   current sensor state when a message is received on this topic
 *    "state_topic":"EnigmaIOT/mains/data",  // string REQUIRED. The MQTT topic subscribed to receive sensor values
 *    "unit_of_measurement":"W"",            // string (optional). Defines the units of measurement of the sensor, if any
 *    "value_template":"{{value_json.pow}}"  // template (optional). Defines a template to extract the value
 * }
 *
 * Template message for sensor is this
 *
 *  Topic:
 *    homeassistant/sensor/<node_name>_<name_suffix>/config
 *
 * Payload
 *
 * ```
 *  {
 *    "name":<node_name>_<name_suffix>,
 *    "unique_id":<node_name>_<name_suffix>,
 *    "device_class": <device_class>,
 *    "expire_after":<expire_time>,
 *    "json_attributes_template":"{{value_json | tojson}}",
 *    "json_attributes_topic":"<network_name>/<node_name>/data",
 *    "state_topic":"<network_name>/<node_name>/data",
 *    "unit_of_measurement":<unit_of_measurement>,
 *    "value_template":"{{value_json.<value_field>}}"
 * }
 * ```
 *
 * Message to gateway is like following
 *
 * ```
 * {
 *    "dev_cla":<device_class>,
 *    "exp_aft":<expire_time>,
 *    "unit_of_meas":<unit_of_measurement>,
 *    "val":<value_field>,
 *    "nmsfx":<name_suffix>
 * }
 * ```
 */

class HASensor : public HAEntity {

public:
    /**
     * @brief Sensor constructor
     */
    HASensor () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = SENSOR;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    /**
     * @brief Define sensor class as `haSensorClass_t`
     *          https://www.home-assistant.io/integrations/sensor.mqtt/#device_class
     * @param devClass Device class
     */
    void setDeviceClass (haSensorClass_t devClass);

    /**
     * @brief Defines the number of seconds after the sensor’s state expires, if it’s not updated. After expiry, the sensor’s state becomes unavailable.
     *          https://www.home-assistant.io/integrations/sensor.mqtt/#expire_after
     * @param payload Expiration value in seconds
     */
    void setExpireTime (uint payload);

    /**
     * @brief Set unit of measure
     *          https://www.home-assistant.io/integrations/sensor.mqtt/#unit_of_measurement
     * @param payload Measure unit
     */
    void setUnitOfMeasurement (const char* payload);

    /**
     * @brief Defines a json key that defines sensor value
     *          https://www.home-assistant.io/integrations/sensor.mqtt/#value_template
     * @param payload ON state value
     */
    void setValueField (const char* payload);

     /**
     * @brief Allows Gateway to get Home Assistant discovery message using Sensor template
     * @param buffer Buffer to hold message string
     * @param buflen Buffer size
     * @param nodeName Originating node name
     * @param networkName EnigmaIOT network name
     * @param inputJSON JSON object sent by node with needed data to fill template in
     * @return Discovery message payload
     */
    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON);

    /**
     * @brief Gets sensor class name from `haSensorClass_t`
     *          https://www.home-assistant.io/integrations/sensor.mqtt/#device_class
     * @param sensorClass Binary sensor class code
     */
    static String deviceClassStr (haSensorClass_t sensorClass);

};

#endif // SUPPORT_HA_DISCOVERY

#endif // _HA_COVER_h