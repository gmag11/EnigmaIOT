/**
  * @file espnow_hal.cpp
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief ESP-NOW communication system abstraction layer. To be used on ESP8266 or ESP32 platforms
  */

#include "espnow_hal.h"
extern "C" {
#include <espnow.h>
}


Espnow_halClass Espnow_hal;

void Espnow_halClass::initComms (peerType_t peerType)
{
    if (esp_now_init () != 0) {
        ESP.restart ();
        delay (1);
    }
    if (peerType == COMM_SENSOR) {
        esp_now_set_self_role (ESP_NOW_ROLE_CONTROLLER);
        esp_now_add_peer (gateway, ESP_NOW_ROLE_SLAVE, channel, NULL, 0);
    } else {
        esp_now_set_self_role (ESP_NOW_ROLE_SLAVE);
    }

    esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
    esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

}

void Espnow_halClass::rx_cb (u8 * mac_addr, u8 * data, u8 len)
{
    if (Espnow_hal.dataRcvd) {
        Espnow_hal.dataRcvd (mac_addr, data, len);
    }
}

void Espnow_halClass::tx_cb (u8 * mac_addr, u8 status)
{
    if (Espnow_hal.sentResult) {
        Espnow_hal.sentResult (mac_addr, status);
    }
}

void Espnow_halClass::begin (uint8_t* gateway, uint8_t channel, peerType_t peerType)
{
    if (peerType == COMM_SENSOR) {
        memcpy (this->gateway, gateway, 6);
        this->channel = channel;
    }
    initComms (peerType);
}

uint8_t Espnow_halClass::send (u8 * da, u8 * data, int len)
{
    return esp_now_send (da, data, len);
}

void Espnow_halClass::onDataRcvd (comms_hal_rcvd_data dataRcvd)
{
    this->dataRcvd = dataRcvd;
}

void Espnow_halClass::onDataSent (comms_hal_sent_data sentResult)
{
    this->sentResult = sentResult;
}
