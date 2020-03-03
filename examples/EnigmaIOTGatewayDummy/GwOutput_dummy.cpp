/**
  * @file GwOutput_dummy.cpp
  * @version 0.8.2
  * @date 03/03/2020
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
}


void GatewayOutput_dummy::loop () {

}

bool GatewayOutput_dummy::outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type) {
    DEBUG_WARN ("Output data send. Address %s. Data %.*s", address, length, data);
}

bool GatewayOutput_dummy::outputControlSend (char* address, uint8_t* data, uint8_t length) {
    DEBUG_INFO ("Output control send. Address %s. Data %s", address, printHexBuffer(data, length));
}

bool GatewayOutput_dummy::newNodeSend (char* address, uint16_t node_id) {
    DEBUG_WARN ("New node: %s NodeID: %d", address, node_id);
}

bool GatewayOutput_dummy::nodeDisconnectedSend (char* address, gwInvalidateReason_t reason) {
    DEBUG_WARN ("Node %s disconnected. Reason %d", address, reason);
}