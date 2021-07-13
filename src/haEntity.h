/**
  * @file haEntity.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief Defines an entity for Home Assistant autodiscovery
  *
  *        https://www.home-assistant.io/docs/mqtt/discovery/
  */

#ifndef _HA_ENTITY_h
#define _HA_ENTITY_h

#include <Arduino.h>
#include "EnigmaIoTconfig.h"

#if SUPPORT_HA_DISCOVERY

#include <ArduinoJson.h>
#include <EnigmaIOTdebug.h>
#include <helperFunctions.h>

constexpr auto ha_device_type = "type";
constexpr auto ha_device_class = "dev_cla";
constexpr auto ha_payload_on = "pl_on";
constexpr auto ha_payload_off = "pl_off";
constexpr auto ha_value_key = "val";
constexpr auto ha_value_template = "val_tpl";
constexpr auto ha_expiration = "exp_aft";
constexpr auto ha_payload_open = "pl_open";
constexpr auto ha_payload_close = "pl_cls";
constexpr auto ha_payload_stop = "pl_stop";
constexpr auto ha_set_position_template = "set_pos_tpl";
constexpr auto ha_position_open = "pos_open";
constexpr auto ha_position_closed = "pos_clsd";
constexpr auto ha_payload_goto = "pl_goto"; // custom
constexpr auto ha_payload = "pl";
constexpr auto ha_state_open = "stat_open";
constexpr auto ha_state_opening = "stat_opening";
constexpr auto ha_state_closed = "stat_clsd";
constexpr auto ha_state_closing = "stat_closing";
constexpr auto ha_state_stopped = "stat_stopped";
constexpr auto ha_state_on = "stat_on";
constexpr auto ha_state_off = "stat_off";
constexpr auto ha_off_delay = "off_dly";
constexpr auto ha_unit_of_measurement = "unit_of_meas";
constexpr auto ha_name_sufix = "nmsfx";
constexpr auto ha_allow_attrib = "attr";
constexpr auto ha_type = "ttype";
constexpr auto ha_subtype = "tstype";


typedef enum {
    UNDEFINED,
    ALARM_PANEL,
    BINARY_SENSOR,
    CAMERA,
    COVER,
    DEVICE_TRACKER,
    DEVICE_TRIGGER,
    FAN,
    HVAC,
    LIGHT,
    LOCK,
    SCENE,
    SENSOR,
    SWITCH,
    TAG_SCANNER,
    VACUUM
} haDeviceType_t; ///< @brief HomeAssistant entity type


typedef enum {
    bs_none,             // Generic on / off.This is the defaultand doesn’t need to be set.
    bs_battery,          // on means low,                     off means normal
    bs_battery_charging, // on means charging,                off means not charging
    bs_cold,             // on means cold,                    off means normal
    bs_connectivity,     // on means connected,               off means disconnected
    bs_door,             // on means open,                    off means closed
    bs_garage_door,      // on means open,                    off means closed
    bs_gas,              // on means gas detected,            off means no gas (clear)
    bs_heat,             // on means hot,                     off means normal
    bs_light,            // on means light detected,          off means no light
    bs_lock,             // on means open (unlocked),         off means closed (locked)
    bs_moisture,         // on means moisture detected (wet), off means no moisture (dry)
    bs_motion,           // on means motion detected,         off means no motion (clear)
    bs_moving,           // on means moving,                  off means not moving (stopped)
    bs_occupancy,        // on means occupied,                off means not occupied (clear)
    bs_opening,          // on means open,                    off means closed
    bs_plug,             // on means device is plugged in,    off means device is unplugged
    bs_power,            // on means power detected,          off means no power
    bs_presence,         // on means home,                    off means away
    bs_problem,          // on means problem detected,        off means no problem (OK)
    bs_safety,           // on means unsafe,                  off means safe
    bs_smoke,            // on means smoke detected,          off means no smoke (clear)
    bs_sound,            // on means sound detected,          off means no sound (clear)
    bs_vibration,        // on means vibration detected,      off means no vibration (clear)
    bs_window            // on means open,                    off means closed
} haBinarySensorClass_t; ///< @brief HomeAssistant Binary Sensor class https://www.home-assistant.io/integrations/binary_sensor/#device-class


typedef enum {
    cover_none,       // Generic cover. This is the default and doesn’t need to be set
    cover_awning,     // Control of an awning, such as an exterior retractable window, door, or patio cover
    cover_blind,      // Control of blinds, which are linked slats that expand or collapse to cover an opening or may be tilted to partially covering an opening, such as window blinds
    cover_curtain,    // Control of curtains or drapes, which is often fabric hung above a window or door that can be drawn open
    cover_damper,     // Control of a mechanical damper that reduces airflow, sound, or light
    cover_door,       // Control of a door or gate that provides access to an area
    cover_garage,     // Control of a garage door that provides access to a garage
    cover_gate,       // Control of a gate. Gates are found outside of a structure and are typically part of a fence
    cover_shade,      // Control of shades, which are a continuous plane of material or connected cells that expanded or collapsed over an opening, such as window shades
    cover_shutter,    // Control of shutters, which are linked slats that swing out / in to covering an opening or may be tilted to partially cover an opening, such as indoor or exterior window shutters
    cover_window      // Control of a physical window that opens and closes or may tilt
} haCoverClass_t; ///< @brief HomeAssistant Cover class https://www.home-assistant.io/integrations/cover/#device-class

typedef enum {
    sensor_none,            // Generic sensor. This is the default and doesn’t need to be set.
    sensor_battery,         // Percentage of battery that is left.
    sensor_current,         // Current in A.
    sensor_energy,          // Energy in Wh or kWh.
    sensor_humidity,        // Percentage of humidity in the air.
    sensor_illuminance,     // The current light level in lx or lm.
    sensor_signal_strength, // Signal strength in dB or dBm.
    sensor_temperature,     // Temperature in °C or °F.
    sensor_power,           // Power in W or kW.
    sensor_power_factor,    // Power factor in % .
    sensor_pressure,        // Pressure in hPa or mbar.
    sensor_timestamp,       // Datetime object or timestamp string (ISO 8601).
    sensor_voltage,         // Voltage in V.
} haSensorClass_t;  ///< @brief HomeAssistant Sensor class https://www.home-assistant.io/integrations/sensor/#device-class

class HAEntity {
protected:
    size_t capacity; ///< @brief JSON object memory reservation length
    haDeviceType_t deviceType = UNDEFINED; ///< @brief HomeAssistant entity device type
    // uint expiration = 0; ///< @brief Entity expiration parameter
    //char name[20]; ///< @brief Entity name
    DynamicJsonDocument* entityConfig; ///< @brief JSON object to be sent to gateway

    /**
     * @brief Default constructor. Needed for inheritance
     */
    HAEntity () {}

public:

    /**
     * @brief Gets entity anounce message to be sent over EnigmaIOT message
     * @param bufferlen Buffer length. Needed legth can be got using `measureMessage ()`
     * @param buffer Buffer to put the payload in
     * @return Amount of data written to buffer
     */
    size_t getAnounceMessage (int bufferlen, uint8_t* buffer) {
#if DEBUG_LEVEL >= WARN
        char* output;
        size_t json_len = measureJsonPretty (*entityConfig) + 1;
        output = (char*)malloc (json_len);
        serializeJsonPretty (*entityConfig, output, json_len);

        DEBUG_DBG ("JSON message\n%s", output);
        free (output);
#endif

        
        if (!buffer) {
            DEBUG_WARN ("Buffer is null");
            return 0;
        }
        //message = buffer;
        int len = measureMsgPack (*entityConfig);

        if (len > MAX_DATA_PAYLOAD_LENGTH) {
            DEBUG_WARN ("Too long message. Reduce HA anounce options");
            return 0;
        }

        if (bufferlen < len) {
            DEBUG_WARN ("Buffer is not big enough");
            return 0;
        }

        len = serializeMsgPack (*entityConfig, buffer, bufferlen);
        DEBUG_DBG ("Msg Pack size: %u", len);

        DEBUG_VERBOSE ("%s", printHexBuffer (buffer, len));

        if (entityConfig) {
            delete (entityConfig);
            entityConfig = NULL;
            DEBUG_DBG ("Deleted JSON");
        }

        return len;
    }
    
    /**
     * @brief Sets name suffix. Used for multi entity nodes
     * @param payload Name suffix
     */
    void setNameSufix (const char* payload) {
       if (payload) {
           (*entityConfig)[ha_name_sufix] = payload;
       }
    }

    /**
     * @brief Enables registering entity attributes as a json object
     */
    void allowSendAttributes () {
        (*entityConfig)[ha_allow_attrib] = true;
    }

    /**
     * @brief Gets needed buffer size for discovery message
     * @return Minimum buffer size
     */
    size_t measureMessage () {
        return measureMsgPack (*entityConfig) + 1;
    }

    /**
     * @brief Gets entity type string from haDeviceType_t value
     *        https://www.home-assistant.io/docs/mqtt/discovery/
     * @param entityType Entity code
     * @return Entity string
     */
    static String deviceTypeStr (haDeviceType_t entityType) {
        switch (entityType) {
        case ALARM_PANEL:
            return "alarm_control_panel";
        case BINARY_SENSOR:
            return "binary_sensor";
        case CAMERA:
            return "camera";
        case COVER:
            return "cover";
        case DEVICE_TRACKER:
            return "device_tracker";
        case DEVICE_TRIGGER:
            return "device_automation";
        case FAN:
            return "fan";
        case HVAC:
            return "climate";
        case LIGHT:
            return "light";
        case LOCK:
            return "lock";
        case SCENE:
            return "scene";
        case SENSOR:
            return "sensor";
        case SWITCH:
            return "switch";
        case TAG_SCANNER:
            return "tag";
        case VACUUM:
            return "vacuum";
        default:
            return "";
        }
    }

    /*
        Discovery configuration topic template
        <hass_prefix>/<device_type>/<node_name>/config
    */
    /**
     * @brief Allows Gateway to get discovery message MQTT topic
     * @param hassPrefix HomeAssistant topic prefix. Usually it is "homeassistant"
     * @param nodeName Name of the node
     * @param entityType Entity type. Used to differentiate discovery message template
     * @param nameSuffix This is used to allow a single node to have different HomeAssistant entities. For instance, a smart switch may behave
     *                  as a power, voltage and current sensor too.
     * @return MQTT topic
     */
    static String getDiscoveryTopic (const char* hassPrefix, const char* nodeName, haDeviceType_t entityType, const char* nameSuffix = NULL) {
        String output;

        if (!hassPrefix) {
            DEBUG_WARN ("Empty prefix");
            return "";
        }
        if (!nodeName) {
            DEBUG_WARN ("Empty node name");
            return "";
        }

        if (nameSuffix) {
            output = String (hassPrefix) + "/" + deviceTypeStr (entityType) + "/" + String (nodeName) + "_" + String(nameSuffix) + "/config";
        } else {
            output = String (hassPrefix) + "/" + deviceTypeStr (entityType) + "/" + String (nodeName) + "/config";        
        }

        return output;
    }


};

#endif // SUPPORT_HA_DISCOVERY

#endif // _HA_ENTITY_h