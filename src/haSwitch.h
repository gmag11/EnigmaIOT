/**
  * @file haSwitch.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief Home Assistant switch integration
  */

#ifndef _SWITCH_h
#define _SWITCH_h

#include "haEntity.h"

#if SUPPORT_HA_DISCOVERY

/**
 * The mqtt switch platform lets you control your MQTT enabled switches. In an ideal scenario, the MQTT device will have a state_topic to publish state changes.
 * If these messages are published with a RETAIN flag, the MQTT switch will receive an instant state update after subscription, and will start with the correct state.
 * Otherwise, the initial state of the switch will be false / off.
 *
 * When a state_topic is not available, the switch will work in optimistic mode. In this mode, the switch will immediately change state after every command.
 * Otherwise, the switch will wait for state confirmation from the device (message from state_topic).
 *
 * Optimistic mode can be forced, even if the state_topic is available. Try to enable it, if experiencing incorrect switch operation.
 *
 * https://www.home-assistant.io/integrations/switch.mqtt/
 *
 * An example of switch discovery message may be like this:
 *
 * Topic:
 *    homeassistant/switch/lights_switch/config
 *
 * Payload:
 *
 * {
 *    "name":"light_switch",                        // string (optional) The name of the device
 *    "unique_id":"light_switch",                   // string (optional) An ID that uniquely identifies this switch device. If two switches have the same unique ID,
 *                                                  //   Home Assistant will raise an exception
 *    "command_topic":"EnigmaIOT/light/set/data",   //  string (optional) The MQTT topic to publish commands to change the switch state
 *    "payload_off":"{\"cmd\":\"swi\",\"swi\":0}",  // string (optional, default: OFF) The payload that represents off state. If specified, will be used for both
 *                                                  //   comparing to the value in the state_topic (see value_template and state_off for details) and sending as off command to the command_topic
 *    "payload_on":"{\"cmd\":\"swi\",\"swi\":1}",   // string (optional, default: ON) The payload that represents on state. If specified, will be used for both comparing to the value in the
 *                                                  //   state_topic (see value_template and state_on for details) and sending as on command to the command_topic
 *    "state_topic":"EnigmaIOT/light/data",         // string (optional) The MQTT topic subscribed to receive state updates
 *    "state_off":0,                                // string (optional) The payload that represents the off state. Used when value that represents off state in the state_topic is different
 *                                                  //   from value that should be sent to the command_topic to turn the device off. Default: payload_off if defined, else OFF
 *    "state_on":1,                                 // string (optional) The payload that represents the on state. Used when value that represents on state in the state_topic is different
 *                                                  //   from value that should be sent to the command_topic to turn the device on. Default: payload_on if defined, else ON
 *    "value_template":"{{value_json.swi}}"         // string (optional) Defines a template to extract device’s state from the state_topic. To determine the switches’s state result of this
 *                                                  //   template will be compared to state_on and state_off
 *    "json_attributes_template":"{{value_json | tojson}}",  // template (optional) Defines a template to extract the JSON dictionary from messages received on the json_attributes_topic.
 *                                                           //   Usage example can be found in MQTT sensor documentation
 *    "json_attributes_topic":"EnigmaIOT/light/data",        // string (optional) The MQTT topic subscribed to receive a JSON dictionary payload and then set as sensor attributes.
 *                                                           //   Usage example can be found in MQTT sensor documentation
 * }
 *
 * Template message for switch is this
 *
 *  Topic:
 *    homeassistant/switch/<node_name>_<name_suffix>/config
 *
 * Payload
 *
 * ```
 *  {
 *    "name":"<node_name>_<name_suffix>",
 *    "unique_id":"<node_name>_<name_suffix>",
 *    "command_topic":"<network_name>/<node_name>/set/data"
 *    "payload_off":"<cmd_payload_on>",
 *    "payload_on":"<cmd_payload_off>"
 *    "state_topic":"<network_name>/<node_name>/data",
 *    "state_off":"<state_off>",
 *    "state_on":"<state_on>",
 *    "value_template":"{{value_json.<value_field>}}"
 *    "json_attributes_template":"{{value_json | tojson}}",
 *    "json_attributes_topic":"<network_name>/<node_name>/data",
 * }
 * ```
 *
 * Message to gateway is like following
 *
 * ```
 * {
 *    "nmsfx":<name_suffix>,
 *    "pl_on":<cmd_payload_on>,
 *    "pl_off":<cmd_payload_off>,
 *    "stat_on":<state_on>,
 *    "stat_off":<state_off>,
 *    "val":<value_field>
 * }
 * ```
 */

class HASwitch : public HAEntity {

public:
    /**
     * @brief Switch constructor
     */
    HASwitch () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = SWITCH;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    /**
     * @brief The payload that represents on state. If specified, will be used for both comparing to the value in the state_topic (see value_template and state_on for details) and sending as on command to the command_topic
     *          https://www.home-assistant.io/integrations/switch.mqtt/#payload_on
     * @param payload ON state string
     */
    void setPayloadOn (const char* payload);

    /**
     * @brief The payload that represents off state. If specified, will be used for both comparing to the value in the state_topic (see value_template and state_off for details) and sending as on command to the command_topic
     *          https://www.home-assistant.io/integrations/switch.mqtt/#payload_off
     * @param payload ON state string
     */
    void setPayloadOff (const char* payload);

    /**
     * @brief The payload that represents on state. If specified, will be used for both comparing to the value in the state_topic (see value_template and state_on for details) and sending as on command to the command_topic
     *          https://www.home-assistant.io/integrations/switch.mqtt/#payload_on
     * @param payload ON state integer
     */
    void setPayloadOn (int payload);

    /**
     * @brief The payload that represents off state. If specified, will be used for both comparing to the value in the state_topic (see value_template and state_off for details) and sending as on command to the command_topic
     *          https://www.home-assistant.io/integrations/switch.mqtt/#payload_off
     * @param payload ON state string
     */
    void setPayloadOff (int payload);

    /**
     * @brief The payload that represents the on state. Used when value that represents on state in the state_topic is different from value that should be sent to the command_topic to turn the device on
     *          https://www.home-assistant.io/integrations/switch.mqtt/#state_on
     * @param payload ON state integer
     */
    void setStateOn (const char* payload);

    /**
     * @brief The payload that represents the off state. Used when value that represents off state in the state_topic is different from value that should be sent to the command_topic to turn the device off
     *          https://www.home-assistant.io/integrations/switch.mqtt/#state_off
     * @param payload ON state integer
     */
    void setStateOff (const char* payload);

    /**
     * @brief The payload that represents the on state. Used when value that represents on state in the state_topic is different from value that should be sent to the command_topic to turn the device on
     *          https://www.home-assistant.io/integrations/switch.mqtt/#state_on
     * @param payload ON state integer
     */
    void setStateOn (int payload);

    /**
     * @brief The payload that represents the off state. Used when value that represents off state in the state_topic is different from value that should be sent to the command_topic to turn the device off
     *          https://www.home-assistant.io/integrations/switch.mqtt/#state_off
     * @param payload ON state integer
     */
    void setStateOff (int payload);

    /**
     * @brief Defines a json key to extract device’s state from the state_topic. To determine the switches’s state result of this template will be compared to state_on and state_off
     *          https://www.home-assistant.io/integrations/switch.mqtt/#value_template
     * @param payload ON state value
     */
    void setValueField (const char* payload);

    /**
     * @brief Allows Gateway to get Home Assistant discovery message using Switch template
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

#endif // _SWITCH_h