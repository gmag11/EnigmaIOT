#include "haCover.h"

void HACover::setDeviceClass (haCoverClass_t devClass) {
    if (devClass > cover_none) {
        // DEBUG_WARN ("Set device class to %d", devClass);
        (*entityConfig)[ha_device_class] = devClass;
    }
}

void HACover::setPayloadOpen (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_open] = payload;
    }
}

void HACover::setPayloadClose (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_close] = payload;
    }
}

void HACover::setPayloadStop (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_stop] = payload;
    }
}

void HACover::setPayloadGoto (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_goto] = payload;
    }
}

void HACover::setStateOpen (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_open] = payload;
    }
}

void HACover::setStateOpening (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_opening] = payload;
    }
}

void HACover::setStateClosed (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_closed] = payload;
    }
}

void HACover::setStateClosing (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_closing] = payload;
    }
}

void HACover::setStateStopped (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_state_stopped] = payload;
    }
}

/* Discovery JSON template for binary sensor
{
    "dev_cla":<device_class>,
    "pl_cls":<pl_cls>>,
    "pl_open":<pl_open>,
    "pl_stop":<pl_stop>,
    "pl_goto":<pl_goto>,
    "pos_open":<pos_open>,
    "pos_clsd":<pos_clsd>,
    "set_pos_tpl":<pl_goto>,
    "stat_clsd":<stat_clsd>,
    "stat_closing":<stat_closing>,
    "stat_open":<stat_open>,
    "stat_opening":<stat_opening>,
    "stat_stopped":<stat_stopped>,
    "nmsfx":<name_suffix>
}
{
    "name":<node_name>_<name_suffix>,
    "unique_id":<node_name>_<name_suffix>,
    "command_topic":<network_name>/<node_name>/set/data,
    "device_class":<device_class>,
    "json_attributes_topic":"<network_name>/<node_name>/data",
    "json_attributes_template":"{{value_json}}",
    "payload_close":"{\"cmd\":<pl_cls>}",
    "payload_open":"{\"cmd\":<pl_open>}",
    "payload_stop":"{\"cmd\":<pl_stop>}",
    "position_topic":"<network_name>/<node_name>/data",
    "position_template":"{{value_json.pos}}",
    "set_position_topic":"<network_name>/<node_name>/set/data,
    "set_position_template":"{\"cmd\":<pl_goto>,\"pos\":{{position|int}}}",
    "state_closed":<stat_clsd>,
    "state_closing":<stat_closing>,
    "state_open":<stat_open>,
    "state_opening":<stat_opening>,
    "state_stopped":<stat_stopped>,
    "state_topic":"<network_name>/<node_name>/data",
    "value_template":"{{value_json.state}}",
}
*/

size_t HACover::getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON) {
    //DynamicJsonDocument inputJSON (1024);
    DynamicJsonDocument outputJSON (1300);

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
    outputJSON["command_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    if (inputJSON->containsKey (ha_device_class)) {
        outputJSON["device_class"] = deviceClassStr ((*inputJSON)[ha_device_class]);
    }
    if (inputJSON->containsKey (ha_payload_close)) {
        outputJSON["payload_close"] = (*inputJSON)[ha_payload_close];
    }
    if (inputJSON->containsKey (ha_payload_open)) {
        outputJSON["payload_open"] = (*inputJSON)[ha_payload_open];
    }
    if (inputJSON->containsKey (ha_payload_stop)) {
        outputJSON["payload_stop"] = (*inputJSON)[ha_payload_stop];
    }
    outputJSON["position_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    outputJSON["position_template"] = "{{value_json.pos}}";
    outputJSON["set_position_topic"] = String (networkName) + "/" + String (nodeName) + "/set/data";
    String pl_goto = "pos";
    if (inputJSON->containsKey (ha_payload_goto)) {
        pl_goto = (*inputJSON)[ha_payload_goto].as<String> ();
    }
    outputJSON["set_position_template"]= String("{\"cmd\":") + pl_goto + String("\"pos\":{{position|int}}}");
    if (inputJSON->containsKey (ha_state_closed)) {
        outputJSON["state_closed"] = (*inputJSON)[ha_state_closed];
    }
    if (inputJSON->containsKey (ha_state_closing)) {
        outputJSON["state_closing"] = (*inputJSON)[ha_state_closing];
    }
    if (inputJSON->containsKey (ha_state_open)) {
        outputJSON["state_open"] = (*inputJSON)[ha_state_open];
    }
    if (inputJSON->containsKey (ha_state_opening)) {
        outputJSON["state_opening"] = (*inputJSON)[ha_state_opening];
    }
    if (inputJSON->containsKey (ha_state_stopped)) {
        outputJSON["state_stopped"] = (*inputJSON)[ha_state_stopped];
    }
    outputJSON["state_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    outputJSON["value_template"] = "{{value_json.state}}";

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

String HACover::deviceClassStr (haCoverClass_t sensorClass) {
    switch (sensorClass) {
    case cover_awning:
        return "awning";
    case cover_blind:
        return "blind";
    case cover_curtain:
        return "curtain";
    case cover_damper:
        return "damper";
    case cover_door:
        return "door";
    case cover_garage:
        return "garage";
    case cover_gate:
        return "gate";
    case cover_shade:
        return "shade";
    case cover_shutter:
        return "shutter";
    case cover_window:
        return "window";
    default:
        return "";
    }
}
