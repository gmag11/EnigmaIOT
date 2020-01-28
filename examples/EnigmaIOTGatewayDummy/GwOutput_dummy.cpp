/**
  * @file GwOutput_dummy.cpp
  * @version 0.8.1
  * @date 17/01/2020
  * @author German Martin
  * @brief Dummy Gateway output module
  *
  * Module to serve as start of EnigmaIOT gateway projects
  */

#include <Arduino.h>
#include "GwOutput_dummy.h"
#include <ESPAsyncWebServer.h>
#include <helperFunctions.h>
#include <debug.h>

#ifdef ESP32
#include <SPIFFS.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_tls.h"
#elif defined(ESP8266)
#include <Hash.h>
#endif // ESP32

#include <FS.h>


GatewayOutput_dummy GwOutput;

typedef struct {
    char address[18];
    int led;
    time_t led_start = 0;
} olr_controller_t;

olr_controller_t controllers[NUM_NODES];

int leds[NUM_NODES] = { 12,13,14,27 };

void GatewayOutput_dummy::configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw) {

}

bool GatewayOutput_dummy::saveConfig () {

}

bool GatewayOutput_dummy::loadConfig () {

}


void GatewayOutput_dummy::configManagerExit (bool status) {
    
}

bool GatewayOutput_dummy::begin () {
    DEBUG_INFO ("Begin");
    for (int i = 0; i < NUM_NODES; i++) {
        controllers[i].led = leds[i];
        pinMode (leds[i], OUTPUT);
        digitalWrite (leds[i], LOW);
    }
}

int findLed (char* address) {
    for (int i = 0; i < NUM_NODES; i++) {
        if (!strcmp (address, controllers[i].address)) {
            return i;
        }
    }
    return -1;
}


void GatewayOutput_dummy::loop () {
    const int LED_ON_TIME = 10;
    for (int i = 0; i < NUM_NODES; i++) {
        if (digitalRead (controllers[i].led)) {
            if (millis () - controllers[i].led_start > LED_ON_TIME) {
                digitalWrite (controllers[i].led, LOW);
            }
        }
    }

}

bool GatewayOutput_dummy::outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type) {
    int node_id = findLed (address);
    DEBUG_INFO ("Output data send. Address %s. Data %.*s", address, length, data);
    //DEBUG_WARN ("Node_id data: %d", node_id);
    digitalWrite (controllers[node_id].led, HIGH);
    controllers[node_id].led_start = millis ();
}

bool GatewayOutput_dummy::outputControlSend (char* address, uint8_t* data, uint8_t length) {
    DEBUG_INFO ("Output control send. Address %s. Data %s", address, printHexBuffer(data, length));
}

bool GatewayOutput_dummy::newNodeSend (char* address, uint16_t node_id) {
    DEBUG_WARN ("New node: %s NodeID: %d", address, node_id);
    if (node_id < NUM_NODES) {
        strlcpy (controllers[node_id].address, address, 18);
    }
    //DEBUG_WARN ("Controller: %s, Node: %s", controllers[node_id].address, address);
    //digitalWrite (controllers[node_id].led, HIGH);
}

bool GatewayOutput_dummy::nodeDisconnectedSend (char* address, gwInvalidateReason_t reason) {
    DEBUG_WARN ("Node %s disconnected. Reason %d", address, reason);
}