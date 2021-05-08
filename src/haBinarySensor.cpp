#include "haBinarySensor.h"

#if SUPPORT_HA_DISCOVERY

void HABinarySensor::setDeviceClass (haBinarySensorClass_t devClass) {
    if (devClass > bs_none) {
        // DEBUG_WARN ("Set device class to %d", devClass);
        (*entityConfig)[ha_device_class] = devClass;
    }
}

void HABinarySensor::setPayloadOn (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_on] = payload;
    }
}

void HABinarySensor::setPayloadOff (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_off] = payload;
    }
}

void HABinarySensor::setPayloadOn (int payload) {
    (*entityConfig)[ha_payload_on] = payload;
}

void HABinarySensor::setPayloadOff (int payload) {
    (*entityConfig)[ha_payload_off] = payload;
}

void HABinarySensor::setValueField (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_value_key] = payload;
    }
}

void HABinarySensor::setValueTemplate (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_value_template] = payload;
    }
}

void HABinarySensor::setOffDelay (uint payload) {
    (*entityConfig)[ha_off_delay] = payload;
}

/* Discovery JSON template for binary sensor
{
  "pl_on":<cmd_payload_on>,
  "pl_off":<cmd_payload_off>,
  "val":<value_field>,
  "exp_aft":<expire_time>,
  "dev_cla":<device_class>
  "nmsfx":<name_suffix>
}

{
  "name":<node_name>_<name_suffix>,
  "unique_id":<node_name>_<name_suffix>,
  "device_class":<device_class>,
  "state_topic":"<network_name>/<node_name>/data",
  "payload_on":<cmd_payload_on>,
  "payload_off":<cmd_payload_off>,
  "value_template":"{{value_json.<value_field>}}",
  "expire_after":<expire_time>
}
*/

size_t HABinarySensor::getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON) {
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
    outputJSON["state_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    if (inputJSON->containsKey (ha_device_class)) {
        outputJSON["device_class"] = deviceClassStr ((*inputJSON)[ha_device_class]);
    }
    if (inputJSON->containsKey (ha_payload_on)) {
        outputJSON["payload_on"] = (*inputJSON)[ha_payload_on];
    }
    if (inputJSON->containsKey (ha_payload_off)) {
        outputJSON["payload_off"] = (*inputJSON)[ha_payload_off];
    }
    if (inputJSON->containsKey (ha_value_template)) {
        String templ = ((*inputJSON)[ha_value_template]).as<String> ();
        templ.replace ("***", nodeName);
        outputJSON["value_template"].set<String> (templ);
    } else if (inputJSON->containsKey (ha_value_key) && (*inputJSON)[ha_value_key].is<String> ()) {
        outputJSON["value_template"] = String ("{{value_json.") + (*inputJSON)[ha_value_key].as<String> () + String ("}}");
    } else {
        outputJSON["value_template"] = "{{value_json.value}}";
    }
    if (inputJSON->containsKey (ha_expiration) && (*inputJSON)[ha_expiration].is<int> ()) {
        outputJSON["expire_after"] = (*inputJSON)[ha_expiration].as<int> ();
    }
    if (inputJSON->containsKey (ha_off_delay) && (*inputJSON)[ha_off_delay].is<int> ()) {
        outputJSON["off_delay"] = (*inputJSON)[ha_off_delay].as<int> ();
    }
    if (inputJSON->containsKey (ha_allow_attrib) && (*inputJSON)[ha_allow_attrib].as<bool> ()) {
        outputJSON["json_attributes_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
        outputJSON["json_attributes_template"] = "{{value_json | tojson}}";
    }

    size_t jsonLen = measureJson (outputJSON);

    if (jsonLen > buflen) {
        DEBUG_WARN ("Too small buffer. Required %u Has %u", jsonLen, buflen);
        return 0;
    }

    //buffer[jsonLen - 1] = '\0';
    serializeJson (outputJSON, buffer, 1024);

    return jsonLen;
}

String HABinarySensor::deviceClassStr (haBinarySensorClass_t sensorClass) {
    switch (sensorClass) {
    case bs_battery:
        return "battery";
    case bs_battery_charging:
        return "battery_charging";
    case bs_cold:
        return "cold";
    case bs_connectivity:
        return "connectivity";
    case bs_door:
        return "door";
    case bs_garage_door:
        return "garage_door";
    case bs_gas:
        return "gas";
    case bs_heat:
        return "heat";
    case bs_light:
        return "light";
    case bs_lock:
        return "lock";
    case bs_moisture:
        return "moisture";
    case bs_motion:
        return "motion";
    case bs_moving:
        return "moving";
    case bs_occupancy:
        return "occupancy";
    case bs_opening:
        return "opening";
    case bs_plug:
        return "plug";
    case bs_power:
        return "power";
    case bs_presence:
        return "presence";
    case bs_problem:
        return "problem";
    case bs_safety:
        return "safety";
    case bs_smoke:
        return "smoke";
    case bs_sound:
        return "sound";
    case bs_vibration:
        return "vibration";
    case bs_window:
        return "window";
    default:
        return "";
    }
}
#endif