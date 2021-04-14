#ifndef _HA_COVER_h
#define _HA_COVER_h

#include "haEntity.h"

#if SUPPORT_HA_DISCOVERY

/**
 * A cover entity can be in states (open, opening, closed or closing).
 * If a state_topic is configured, the entity’s state will be updated only after an MQTT message is received on
 * state_topic matching state_open, state_opening, state_closed or state_closing. For covers that only report
 * 3 states (opening, closing, stopped), a state_stopped state can be configured to indicate that the device is
 * not moving. When this payload is received on the state_topic, and a position_topic is not configured, the
 * cover will be set to state closed if its state was closing and to state open otherwise. If a position_topic
 * is set, the cover’s position will be used to set the state to either open or closed state.
 * If the cover reports its position, a position_topic can be configured for receiving the position.
 * If no state_topic is configured, the cover’s state will be set to either open or closed when a position is received.
 *
 * https://www.home-assistant.io/integrations/cover.mqtt/
 *
 * An example of cover discovery message may be like this:
 *
 * Topic:
 *    homeassistant/cover/cover-bedroom/config
 *
 * Payload:
 *
 * {
 *   "name":"cover-bedroom",  // string (optional, default: MQTT Cover) The name of the cover.
 *   "unique_id":"cover-bedroom",  // string (optional) An ID that uniquely identifies this cover. If two covers have
 *                                 //   the same unique ID, Home Assistant will raise an exception.
 *   "command_topic":"EnigmaIOT/cover-bedroom/set/data",  // string (optional) The MQTT topic to publish commands to control the cover.
 *   "device_class":"shade",  // string (optional) Sets the class of the device, changing the device state and icon that is displayed on the frontend.
 *                            //   https://www.home-assistant.io/integrations/cover/
 *   "json_attributes_topic":"EnigmaIOT/cover-bedroom/data",  // string (optional) The MQTT topic subscribed to receive a JSON dictionary
 *                                                            //   payload and then set as sensor attributes. Usage example can be found in MQTT sensor documentation
 *   "json_attributes_template":"{{value_json}}",  // template (optional) Defines a template to extract the JSON dictionary from messages received
 *                                                 // on the json_attributes_topic. Usage example can be found in MQTT sensor documentation
 *   "payload_close":"{\"cmd\":\"dd\"}",  // string (optional, default: CLOSE) The command payload that closes the cover
 *   "payload_open":"{\"cmd\":\"uu\"}",   // string (optional, default: OPEN) The command payload that opens the cover.
 *   "payload_stop":"{\"cmd\":\"stop\"}",  // string (optional, default: STOP) The command payload that stops the cover.
 *   "position_topic":"EnigmaIOT/cover-bedroom/data",  // string (optional) The MQTT topic subscribed to receive cover position messages
 *   "position_template":"{{value_json.pos}}",  // string (optional) Defines a template that can be used to extract the payload for the position_topic topic
 *   "set_position_topic":"EnigmaIOT/cover-bedroom/set/data,  // string (optional) The MQTT topic to publish position commands to.
 *                                                               //  You need to set position_topic as well if you want to use position topic.
 *                                                               //  Use template if position topic wants different values than within range position_closed - position_open.
 *                                                               //  If template is not defined and position_closed != 100 and position_open != 0 then proper position
 *                                                               //  value is calculated from percentage position
 *   "set_position_template":"{\"cmd\":\"go\",\"pos\":{{position|int}}}",  // string (optional) Defines a template to define the position to be sent to the
 *                                                                            //  set_position_topic topic. Incoming position value is available for use in the template ``.
 *                                                                            //  If no template is defined, the position (0-100) will be calculated according to
 *                                                                            //  position_open and `position_closed` values
 *   "state_closed":"CLOSED"",  // string (optional, default: closed) The payload that represents the closed state
 *   "state_closing":"CLOSING",  // string (optional, default: closing) The payload that represents the closing state
 *   "state_open":"OPEN",  // string (optional, default: open) The payload that represents the open state
 *   "state_opening":"OPENING",  // string (optional, default: opening) The payload that represents the opening state
 *   "state_stopped":"STOPPED",  // string (optional, default: stopped) The payload that represents the stopped state (for covers that do not report open/closed state)
 *   "state_topic":"EnigmaIOT/cover-bedroom/data",  //  string (optional) The MQTT topic subscribed to receive cover state messages. State topic can only read (open, opening, closed, closing or stopped) state
 *   "value_template":"{{value_json.state}}",  // string (optional) Defines a template that can be used to extract the payload for the state_topic topic
 * }
 * 
 * Template message for binary sensor is this
 *
 *  Topic:
 *    homeassistant/cover/<node_name>_<name_suffix>/config
 *
 * Payload
 * {
 *   "name":<node_name>_<name_suffix>,
 *   "unique_id":<node_name>_<name_suffix>,
 *   "command_topic":<network_name>/<node_name>/set/data,
 *   "device_class":<device_class>,
 *   "json_attributes_topic":"<network_name>/<node_name>/data",
 *   "json_attributes_template":"{{value_json}}",
 *   "payload_close":"{\"cmd\":<pl_cls>}",
 *   "payload_open":"{\"cmd\":<pl_open>}",
 *   "payload_stop":"{\"cmd\":<pl_stop>}",
 *   "position_topic":"<network_name>/<node_name>/data",
 *   "position_template":"{{value_json.pos}}",
 *   "set_position_topic":"<network_name>/<node_name>/set/data,
 *   "set_position_template":"{\"cmd\":<pl_goto>,\"pos\":{{position|int}}}",
 *   "state_closed":<stat_clsd>,
 *   "state_closing":<stat_closing>,
 *   "state_open":<stat_open>,
 *   "state_opening":<stat_opening>,
 *   "state_stopped":<stat_stopped>,
 *   "state_topic":"<network_name>/<node_name>/data",
 *   "value_template":"{{value_json.state}}",
 * }
 *
 * Message to gateway is like following
 *
 * {
 *   "dev_cla":<device_class>,
 *   "pl_cls":<pl_cls>>,
 *   "pl_open":<pl_open>,
 *   "pl_stop":<pl_stop>,
 *   "pl_goto":<pl_goto>,
 *   "pos_open":<pos_open>,
 *   "pos_clsd":<pos_clsd>,
 *   "set_pos_tpl":<pl_goto>,
 *   "stat_clsd":<stat_clsd>,
 *   "stat_closing":<stat_closing>,
 *   "stat_open":<stat_open>,
 *   "stat_opening":<stat_opening>,
 *   "stat_stopped":<stat_stopped>,
 * }
 *
 */

class HACover : public HAEntity {

public:

    /**
     * @brief Cover constructor
     */
    HACover () {
        capacity = JSON_OBJECT_SIZE (10) + 250;
        entityConfig = new DynamicJsonDocument (capacity);
        deviceType = COVER;
        (*entityConfig)[ha_device_type] = deviceType;
    };

    /**
     * @brief Define binary sensor class as `haCoverClass_t`
     * @param devClass Device class
     */
    void setDeviceClass (haCoverClass_t devClass);
    
    /**
     * @brief The command payload that opens the cover
     *          https://www.home-assistant.io/integrations/cover.mqtt/#payload_open
     * @param payload Open command string
     */
    void setPayloadOpen (const char* payload);

    /**
     * @brief The command payload that closes the cover
     *          https://www.home-assistant.io/integrations/cover.mqtt/#payload_close
     * @param payload Close command string
     */
    void setPayloadClose (const char* payload);

    /**
     * @brief The command payload that stops the cover
     *          https://www.home-assistant.io/integrations/cover.mqtt/#payload_stop
     * @param payload Stop command string
     */
    void setPayloadStop (const char* payload);

    /**
     * @brief The command that moves the cover to specific position
     * @param payload Go to command string
     */
    void setPayloadGoto (const char* payload);

    /**
     * @brief The payload that represents the open state
     *          https://www.home-assistant.io/integrations/cover.mqtt/#state_open
     * @param payload Open state string
     */
    void setStateOpen (const char* payload);
    
    /**
     * @brief The payload that represents the opening state
     *          https://www.home-assistant.io/integrations/cover.mqtt/#state_opening
     * @param payload Opening state string
     */
    void setStateOpening (const char* payload);
    
    /**
     * @brief The payload that represents the closed state
     *          https://www.home-assistant.io/integrations/cover.mqtt/#state_closed
     * @param payload Closed state string
     */
    void setStateClosed (const char* payload);
    
    /**
     * @brief The payload that represents the closing state
     *          https://www.home-assistant.io/integrations/cover.mqtt/#state_closing
     * @param payload Closing state string
     */
    void setStateClosing (const char* payload);
    
    /**
     * @brief The payload that represents the stopped state
     *          https://www.home-assistant.io/integrations/cover.mqtt/#state_stopped
     * @param payload Stopped state string
     */
    void setStateStopped (const char* payload);

    /**
     * @brief Allows Gateway to get Home Assistant discovery message using Cover template
     * @param buffer Buffer to hold message string
     * @param buflen Buffer size
     * @param nodeName Originating node name
     * @param networkName EnigmaIOT network name
     * @param inputJSON JSON object sent by node with needed data to fill template in
     * @return Discovery message payload
     */
    static size_t getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON);

    /**
     * @brief Gets binary sensor class name from `haCoverClass_t`
     *          https://www.home-assistant.io/integrations/cover.mqtt/#device_class
     * @param payload Cover class code
     */
    static String deviceClassStr (haCoverClass_t sensorClass);

};

#endif // SUPPORT_HA_DISCOVERY

#endif // _HA_COVER_h