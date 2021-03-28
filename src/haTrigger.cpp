#include "haTrigger.h"

void HATrigger::setPayload (const char* payload) {
    if (payload) {
        (*entityConfig)[ha_payload_on] = payload;
    }
}

/* Discovery JSON template for trigger
{
    "pl":<payload_value>,
    "type":<type>,
    "stype":<subtype>
}

{
    "automation_type":"trigger",
    "topic":"<network_name>/<node_name>/data",
    "payload":<payload_value>,
    "type":<type>,
    "subtype":<subtype>,
    "device":{
        "name":<node_name>_<type>_<subtype>,
        "identifiers":<node_name>_<type>_<subtype>
    }
}
*/

size_t HATrigger::getDiscoveryJson (char* buffer, size_t buflen, const char* nodeName, const char* networkName, DynamicJsonDocument* inputJSON) {
    //DynamicJsonDocument inputJSON (1024);
    DynamicJsonDocument outputJSON (1024);
    String type;
    String subtype;

    //deserializeMsgPack (inputJSON, msgPack, len);

    if (!nodeName || !networkName || !inputJSON || !buffer/*!msgPack || !len*/) {
        DEBUG_WARN ("Whrong parameters");
        return 0;
    }

    outputJSON["automation_type"] = "trigger";
    outputJSON["command_topic"] = String (networkName) + "/" + String (nodeName) + "/data";
    
    if (inputJSON->containsKey (ha_payload)) {
        outputJSON["payload"] = (*inputJSON)[ha_payload].as<String> ();
    } 
    if (inputJSON->containsKey (ha_type)) {
        if ((*inputJSON)[ha_type].is<int> ()) {
            type = getTriggerTypeStr ((*inputJSON)[ha_type].as<int> ());
            outputJSON["type"] = type;
        } else {
            type = (*inputJSON)[ha_type].as<String>();
            outputJSON["type"] = type;        
        }
    }
    if (inputJSON->containsKey (ha_subtype)) {
        if ((*inputJSON)[ha_subtype].is<int> ()) {
            subtype = getTriggerSubtypeStr ((*inputJSON)[ha_subtype].as<int> ());
            outputJSON["subtype"] = subtype;
        } else {
            subtype = (*inputJSON)[ha_subtype].as<String> ();
            outputJSON["subtype"] = subtype;
        }
    }

    JsonObject device = outputJSON.createNestedObject ("device");
    device["name"] = String (nodeName) + (type != "" ? ("_" + type) : "") + (subtype != "" ? ("_" + subtype) : "");
    device["identifiers"] = device["name"];
    
    size_t jsonLen = measureJson (outputJSON) + 1;

    if (jsonLen > buflen) {
        DEBUG_WARN ("Too small buffer. Required %u Has %u", jsonLen, buflen);
        return 0;
    }

    //buffer[jsonLen - 1] = '\0';
    serializeJson (outputJSON, buffer, buflen);

    return jsonLen;
}

