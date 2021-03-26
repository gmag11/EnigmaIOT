#include "haSwitch.h"

void HASwitch::setPayloadOn (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_on] = payload;
    }
}

void HASwitch::setPayloadOff (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_off] = payload;
    }
}

void HASwitch::setPayloadOn (int payload) {
    (*entityConfig)[ha_payload_on] = payload;
}

void HASwitch::setPayloadOff (int payload) {
    (*entityConfig)[ha_payload_off] = payload;
}

void HASwitch::setStateOn (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_on] = payload;
    }
}

void HASwitch::setStateOff (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_off] = payload;
    }
}

void HASwitch::setStateOn (int payload) {
    (*entityConfig)[ha_state_on] = payload;
}

void HASwitch::setStateOff (int payload) {
    (*entityConfig)[ha_state_off] = payload;
}

void HASwitch::setValueField (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_value_key] = payload;
    }
}

/* Discovery JSON template for binary sensor
{
    "nmsfx":<name_suffix>,
    "pl_on":<cmd_payload_on>,
    "pl_off":<cmd_payload_off>,
    "stat_on":<state_on>,
    "stat_off":<state_off>,
    "val":<value_field>
}

{
    "name":"<node_name>_<name_suffix>",
    "unique_id":"<node_name>_<name_suffix>",
    "command_topic":"<network_name>/<node_name>/set/data"
    "payload_off":"<cmd_payload_on>",
    "payload_on":"<cmd_payload_off>"
    "state_topic":"<network_name>/<node_name>/data",
    "state_off":"<state_off>",
    "state_on":"<state_on>",
    "value_template":"{{value_json.<value_field>}}"
    "json_attributes_template":"{{value_json | tojson}}",
    "json_attributes_topic":"<network_name>/<node_name>/data",
}
*/

size_t HASwitch::getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON) {
    //DynamicJsonDocument inputJSON (1024);
    DynamicJsonDocument outputJSON (1024);

    //deserializeMsgPack (inputJSON, msgPack, len);

    if (!nodeName || !networkName || !inputJSON /*!msgPack || !len*/) {
        DEBUG_WARN ("Whrong parameters");
        return 0;
    }

    if (inputJSON->containsKey (ha_name_sufix)) {
        outputJSON["name"] = String (nodeName) + "_" + (*inputJSON)[ha_name_sufix].as<String> ();
    } else {
        outputJSON["name"] = nodeName;    
    }
    if (inputJSON->containsKey (ha_name_sufix)) {
        outputJSON["unique_id"] = String (nodeName) + "_" + (*inputJSON)[ha_name_sufix].as<String> ();
    } else {
        outputJSON["unique_id"] = nodeName;
    }
    outputJSON["command_topic"] = String (networkName) + "/" + String (nodeName) + "/set/data";
    if (inputJSON->containsKey (ha_payload_on)) {
        outputJSON["payload_on"] = (*inputJSON)[ha_payload_on];
    }
    if (inputJSON->containsKey (ha_payload_off)) {
        outputJSON["payload_off"] = (*inputJSON)[ha_payload_off];
    }
    outputJSON["state_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    if (inputJSON->containsKey (ha_state_on)) {
        outputJSON["state_on"] = (*inputJSON)[ha_state_on];
    }
    if (inputJSON->containsKey (ha_state_off)) {
        outputJSON["state_off"] = (*inputJSON)[ha_state_off];
    } 
    if (inputJSON->containsKey (ha_value_key) && (*inputJSON)[ha_value_key].is<String> ()) {
        outputJSON["value_template"] = String ("{{value_json.") + (*inputJSON)[ha_value_key].as<String> () + String ("}}");
    }
    if (inputJSON->containsKey (ha_allow_attrib) && (*inputJSON)[ha_allow_attrib].as<bool> ()) {
        outputJSON["json_attributes_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
        outputJSON["json_attributes_template"] = "{{value_json | tojson}}";
    }
    
    size_t jsonLen = measureJson (outputJSON) + 1;

    if (jsonLen > buflen) {
        DEBUG_WARN ("Too small buffer. Required %u Has %u", jsonLen, buflen);
        return 0;
    }

    //buffer[jsonLen - 1] = '\0';
    serializeJson (outputJSON, buffer, 1024);

    return jsonLen;
}

