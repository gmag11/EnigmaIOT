/**
  * @file espnow_hal.cpp
  * @version 0.9.2
  * @date 01/07/2020
  * @author German Martin
  * @brief ESP-NOW communication system abstraction layer. To be used on ESP8266 or ESP32 platforms
  */

#include "espnow_hal.h"
extern "C" {
#ifdef ESP8266
#include <espnow.h>
#elif defined ESP32
#include <esp_now.h>
#include <esp_log.h>
#include <esp_wifi.h>
#endif
}


Espnow_halClass Espnow_hal;

peerType_t _peerType;

void Espnow_halClass::initComms (peerType_t peerType) {
	if (esp_now_init () != 0) {
		ESP.restart ();
		delay (1);
	}
	if (peerType == COMM_NODE) {
#ifdef ESP8266
		esp_now_set_self_role (ESP_NOW_ROLE_CONTROLLER);
		esp_now_add_peer (gateway, ESP_NOW_ROLE_SLAVE, channel, NULL, 0);
#elif defined ESP32
		esp_now_peer_info_t networkGw;
		memcpy (networkGw.peer_addr, gateway, COMMS_HAL_ADDR_LEN);
		networkGw.channel = channel;
		networkGw.ifidx = ESP_IF_WIFI_STA;
		networkGw.encrypt = false;
		esp_now_add_peer (&networkGw);
		DEBUG_INFO ("Gateway peer Added");
#endif
	}
#ifdef ESP8266
	else {
		esp_now_set_self_role (ESP_NOW_ROLE_SLAVE);
	}
#endif

	esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
	esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

}

void ICACHE_FLASH_ATTR Espnow_halClass::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
	if (Espnow_hal.dataRcvd) {
		Espnow_hal.dataRcvd (mac_addr, data, len);
	}
}

void ICACHE_FLASH_ATTR Espnow_halClass::tx_cb (uint8_t* mac_addr, uint8_t status) {
	if (Espnow_hal.sentResult) {
		Espnow_hal.sentResult (mac_addr, status);
	}
}

void Espnow_halClass::begin (uint8_t* gateway, uint8_t channel, peerType_t peerType) {
	_ownPeerType = peerType;
	_peerType = peerType;
	if (peerType == COMM_NODE) {
		memcpy (this->gateway, gateway, COMMS_HAL_ADDR_LEN);
		this->channel = channel;
	}
	initComms (peerType);
}

void Espnow_halClass::stop () {
	Serial.println ("-------------> ESP-NOW STOP");
	esp_now_unregister_recv_cb ();
	esp_now_unregister_send_cb ();
	esp_now_deinit ();
}

int32_t Espnow_halClass::send (uint8_t* da, uint8_t* data, int len) {
#ifdef ESP32
	char buffer[18];
	mac2str (da, buffer);
	ESP_LOGD (TAG, "ESP-NOW message to %s", buffer);
	if (_ownPeerType == COMM_GATEWAY) {
		esp_now_peer_info_t peer;
		memcpy (peer.peer_addr, da, COMMS_HAL_ADDR_LEN);
		uint8_t ch;
		wifi_second_chan_t secondCh;
		esp_wifi_get_channel (&ch, &secondCh);
		peer.channel = ch;
		peer.ifidx = ESP_IF_WIFI_AP;
		peer.encrypt = false;
		esp_err_t error = esp_now_add_peer (&peer);
		ESP_LOGD (TAG, "Peer added on channel %u. Result %d", ch, error);

		//wifi_bandwidth_t bw;
		//esp_wifi_get_bandwidth (ESP_IF_WIFI_AP, &bw);
		//ESP_LOGD (TAG, "WiFi bandwidth: %s", bw == WIFI_BW_HT20 ? "20 MHz" : "40 MHz");
	}
#endif

	// Serial.printf ("Phy Mode ---> %d\n", (int)wifi_get_phy_mode ());
	int32_t error = esp_now_send (da, data, len);
#ifdef ESP32
	ESP_LOGD (TAG, "esp now send result = %d", error);
	if (_ownPeerType == COMM_GATEWAY) {
		esp_err_t error = esp_now_del_peer (da);
		ESP_LOGD (TAG, "Peer deleted. Result %d", error);

	}
#endif
	return error;
}

void Espnow_halClass::onDataRcvd (comms_hal_rcvd_data dataRcvd) {
	this->dataRcvd = dataRcvd;
}

void Espnow_halClass::onDataSent (comms_hal_sent_data sentResult) {
	this->sentResult = sentResult;
}
