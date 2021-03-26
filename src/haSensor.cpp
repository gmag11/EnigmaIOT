#include "haSensor.h"

void HASensor::setDeviceClass (haSensorClass_t devClass) {
    if (devClass > sensor_none) {
        // DEBUG_WARN ("Set device class to %d", devClass);
        (*entityConfig)[ha_device_class] = devClass;
    }
}

void HASensor::setExpireTime (uint payload) {
    if (payload) {
        (*entityConfig)[ha_expiration] = payload;
    }
}

void HASensor::setUnitOfMeasurement (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_unit_of_measurement] = payload;
    }
}

void HASensor::setValueField (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_value_key] = payload;
    }
}

/* Discovery JSON template for binary sensor
{
    "dev_cla":<device_class>,
    "exp_aft":<expire_time>,
    "unit_of_meas":<unit_of_measurement>,
    "val":<value_field>,
    "nmsfx":<name_suffix>
}
{
    "name":<node_name>_<name_suffix>,
    "unique_id":<node_name>_<name_suffix>,
    "device_class": <device_class>,
    "expire_after":<expire_time>,
    "json_attributes_template":"{{value_json | tojson}}",
    "json_attributes_topic":"<network_name>/<node_name>/data",
    "state_topic":"<network_name>/<node_name>/data",
    "unit_of_measurement":<unit_of_measurement>,
    "value_template":"{{value_json.<value_field>}}"
}
*/

size_t HASensor::getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON) {
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
    if (inputJSON->containsKey (ha_device_class)) {
        outputJSON["device_class"] = deviceClassStr ((*inputJSON)[ha_device_class]);
    }
    if (inputJSON->containsKey (ha_expiration) && (*inputJSON)[ha_expiration].is<int> ()) {
        outputJSON["expire_after"] = deviceClassStr ((*inputJSON)[ha_expiration]);
    }
    outputJSON["state_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    if (inputJSON->containsKey (ha_unit_of_measurement)) {
        outputJSON["unit_of_measurement"] = deviceClassStr ((*inputJSON)[ha_unit_of_measurement]);
    }
    if (inputJSON->containsKey (ha_value_key)) {
        outputJSON["value_template"] = String ("{{value_json.") + (*inputJSON)[ha_value_key].as<String> () + String ("}}");
    } else {
        outputJSON["value_template"] = "{{value_json.value}}";
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

String HASensor::deviceClassStr (haSensorClass_t sensorClass) {
    switch (sensorClass) {
    case sensor_battery:
        return "battery";
    case sensor_current:
        return "current";
    case sensor_energy:
        return "energy";
    case sensor_humidity:
        return "humidity";
    case sensor_illuminance:
        return "illuminance";
    case sensor_signal_strength:
        return "signal_strength";
    case sensor_temperature:
        return "temperature";
    case sensor_power:
        return "power";
    case sensor_power_factor:
        return "power_factor";
    case sensor_pressure:
        return "pressure";
    case sensor_timestamp:
        return "timestamp";
    case sensor_voltage:
        return "voltage";
    default:
        return "";
    }
}
