/**
  * @file espnow_hal.cpp
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief ESP-NOW communication system abstraction layer. To be used on ESP8266 or ESP32 platforms
  */

#include "espnow_hal.h"
extern "C" {
#ifdef ESP8266
#include <espnow.h>
#elif defined ESP32
#include <esp_now.h>
#include <esp_wifi.h>
#endif
}

typedef struct {
    uint16_t frame_head;
    uint16_t duration;
    uint8_t destination_address[6];
    uint8_t source_address[6];
    uint8_t broadcast_address[6];
    uint16_t sequence_control;

    uint8_t category_code;
    uint8_t organization_identifier[3]; // 0x18fe34
    uint8_t random_values[4];
    struct {
        uint8_t element_id;                 // 0xdd
        uint8_t lenght;                     //
        uint8_t organization_identifier[3]; // 0x18fe34
        uint8_t type;                       // 4
        uint8_t version;
        uint8_t body[0];
    } vendor_specific_content;
} __attribute__ ((packed)) espnow_frame_format_t;

#ifdef ESP8266

typedef struct {
    signed rssi : 8;
    unsigned rate : 4;
    unsigned is_group : 1;
    unsigned : 1;
    unsigned sig_mode : 2;
    unsigned legacy_length : 12;
    unsigned damatch0 : 1;
    unsigned damatch1 : 1;
    unsigned bssidmatch0 : 1;
    unsigned bssidmatch1 : 1;
    unsigned MCS : 7;
    unsigned CWB : 1;
    unsigned HT_length : 16;
    unsigned Smoothing : 1;
    unsigned Not_Sounding : 1;
    unsigned : 1;
    unsigned Aggregation : 1;
    unsigned STBC : 2;
    unsigned FEC_CODING : 1;
    unsigned SGI : 1;
    unsigned rxend_state : 8;
    unsigned ampdu_cnt : 8;
    unsigned channel : 4;
    unsigned : 12;
} wifi_pkt_rx_ctrl_t;

typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t payload[0]; /* ieee80211 packet buff */
} wifi_promiscuous_pkt_t;
#endif

Espnow_halClass Espnow_hal;

peerType_t _peerType;

void Espnow_halClass::initComms (peerType_t peerType) {
	if (esp_now_init ()) {
		ESP.restart ();
		delay (1);
	}
	if (peerType == COMM_NODE) {
#ifdef ESP8266
		esp_now_set_self_role (ESP_NOW_ROLE_CONTROLLER);
		esp_now_add_peer (gateway, ESP_NOW_ROLE_SLAVE, channel, NULL, 0);
#elif defined ESP32
		// esp_now_peer_info_t networkGw;
		// memcpy (networkGw.peer_addr, gateway, COMMS_HAL_ADDR_LEN);
		// networkGw.channel = channel;
        // networkGw.ifidx = WIFI_IF_STA;
		// networkGw.encrypt = false;
        //esp_err_t result = esp_now_add_peer (&networkGw);
		//DEBUG_INFO ("Gateway peer Added in channel %d. Result = %s", channel, esp_err_to_name (result));
		DEBUG_DBG ("WIFI channel is %d", WiFi.channel ());
#endif
	}
#ifdef ESP8266
	else {
		esp_now_set_self_role (ESP_NOW_ROLE_SLAVE);
	}
#endif

	esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
    esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

#ifdef ESP32
    xTaskCreateUniversal (runHandle, "espnow_loop", 2048, NULL, 1, &espnowLoopTask, CONFIG_ARDUINO_RUNNING_CORE);
#else
    os_timer_setfn (&espnowLoopTask, runHandle, NULL);
    os_timer_arm (&espnowLoopTask, 20, true);
    // timer1_attachInterrupt (runHandle);
    // timer1_enable (TIM_DIV16, TIM_EDGE, TIM_LOOP);
    // timer1_write (25000); //5000 us
#endif
}

void ICACHE_FLASH_ATTR Espnow_halClass::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
    //espnow_frame_format_t* espnow_data = (espnow_frame_format_t*)(data - sizeof (espnow_frame_format_t));
    wifi_promiscuous_pkt_t* promiscuous_pkt = (wifi_promiscuous_pkt_t*)(data - sizeof (wifi_pkt_rx_ctrl_t) - sizeof (espnow_frame_format_t));
    wifi_pkt_rx_ctrl_t* rx_ctrl = &promiscuous_pkt->rx_ctrl;
    
    if (Espnow_hal.dataRcvd) {
        Espnow_hal.dataRcvd (mac_addr, data, len, rx_ctrl->rssi - 98); // rssi should be in dBm but it has added almost 100 dB. Do not know why
	}
}

void ICACHE_FLASH_ATTR Espnow_halClass::tx_cb (uint8_t* mac_addr, uint8_t status) {
    Espnow_hal.readyToSend = true;
    DEBUG_DBG ("Ready to send: true");
	if (Espnow_hal.sentResult) {
		Espnow_hal.sentResult (mac_addr, status);
	}
}

void Espnow_halClass::begin (uint8_t* gateway, uint8_t channel, peerType_t peerType) {
	_ownPeerType = peerType;
	_peerType = peerType;
	DEBUG_INFO ("Starting ESP-NOW as %s", _peerType == COMM_GATEWAY ? "gateway" : "node");
	if (peerType == COMM_NODE) {
		DEBUG_DBG ("Gateway address is " MACSTR, MAC2STR (gateway));
		memcpy (this->gateway, gateway, COMMS_HAL_ADDR_LEN);
		this->channel = channel;
	}
	initComms (peerType);
	if (_ownPeerType == COMM_NODE) {
		addPeer (BROADCAST_ADDRESS);
	}
}

bool Espnow_halClass::addPeer (const uint8_t* da) {
#ifdef ESP32
	esp_now_peer_info_t peer;
	memcpy (peer.peer_addr, da, COMMS_HAL_ADDR_LEN);
	uint8_t ch;
	wifi_second_chan_t secondCh;
	esp_wifi_get_channel (&ch, &secondCh);
	peer.channel = ch;
    if (_ownPeerType == COMM_NODE) {
        peer.ifidx = WIFI_IF_STA;
    } else {
        peer.ifidx = WIFI_IF_AP;
    }
	peer.encrypt = false;
	esp_err_t error = esp_now_add_peer (&peer);
	DEBUG_DBG ("Peer %s added on channel %u. Result 0x%X %s", mac2str (da), ch, error, esp_err_to_name (error));
	return error == ESP_OK;
#else 
	return true;
#endif
}

void Espnow_halClass::stop () {
    DEBUG_INFO ("-------------> ESP-NOW STOP");
	esp_now_unregister_recv_cb ();
    esp_now_unregister_send_cb ();
    esp_now_deinit ();
}

int32_t Espnow_halClass::send (uint8_t* da, uint8_t* data, int len) {
    comms_queue_item_t message;

    if (!da || !data || !len) {
        DEBUG_WARN ("Parameters error");
        return -1;
    }
    
    if (len > MAX_MESSAGE_LENGTH) {
        DEBUG_WARN ("Length error");
        return -1;
    }

    if (out_queue.size () >= COMMS_QUEUE_SIZE) {
        out_queue.pop ();
    }

    memcpy (message.dstAddress, da, ENIGMAIOT_ADDR_LEN);
    message.payload_len = len;
    memcpy (message.payload, data, len);
    
    if (out_queue.push (&message)) {
        DEBUG_DBG ("%d Comms messages queued. Type: 0x%02X Len: %d", out_queue.size (), data[0], len);
        return 0;
    } else {
        DEBUG_WARN ("Error queuing Comms message 0x%02X to %s", data[0], mac2str (da));
        return -1;
    }
}

comms_queue_item_t* Espnow_halClass::getCommsQueue () {
    if (out_queue.size ()) {
        DEBUG_DBG ("Comms message got from queue");
        return out_queue.front ();
    }
    return nullptr;
}

void Espnow_halClass::popCommsQueue () {
    if (out_queue.size ()) {
        comms_queue_item_t* message;

        message = out_queue.front ();
        if (message) {
            message->payload_len = 0;
        }
        out_queue.pop ();
        DEBUG_DBG ("Comms message pop. Queue size %d", out_queue.size ());
    }
}

int32_t Espnow_halClass::sendEspNowMessage (comms_queue_item_t* message) {
    int32_t error;

    if (!message) {
        return -1;
    }
    if (!(message->payload_len) || (message->payload_len > MAX_MESSAGE_LENGTH)) {
        return -1;
    }
    
	DEBUG_DBG ("ESP-NOW message to %s", mac2str(message->dstAddress));
#ifdef ESP32
	//if (_ownPeerType == COMM_GATEWAY) {
        addPeer (message->dstAddress);
        DEBUG_DBG ("Peer added");
    //}
#endif

    DEBUG_DBG ("Ready to send: false");
    readyToSend = false;
    error = esp_now_send (message->dstAddress, message->payload, message->payload_len);
#ifdef ESP32
    DEBUG_DBG ("esp now send result = %s", esp_err_to_name(error));
	//if (_ownPeerType == COMM_GATEWAY) {
        error = esp_now_del_peer (message->dstAddress); // TODO: test
        DEBUG_DBG ("Peer deleted. Result %s", esp_err_to_name(error));
	//}
#endif
	return error;
}

void Espnow_halClass::onDataRcvd (comms_hal_rcvd_data dataRcvd) {
	this->dataRcvd = dataRcvd;
}

void Espnow_halClass::onDataSent (comms_hal_sent_data sentResult) {
	this->sentResult = sentResult;
}

void Espnow_halClass::handle () {
    if (readyToSend) {
        //DEBUG_WARN ("Process queue: Elements: %d", out_queue.size ());
        if (!out_queue.empty ()) {
            comms_queue_item_t* message;
            message = getCommsQueue ();
            if (message) {
                if (!sendEspNowMessage (message)) {
                    DEBUG_DBG ("Message to %s sent. Type: 0x%02X. Len: %u", mac2str (message->dstAddress), (message->payload)[0], message->payload_len);
                } else {
                    DEBUG_WARN ("Error sendign message to %s. Type: 0x%02X. Len: %u", mac2str (message->dstAddress), (message->payload)[0], message->payload_len);
                }
                popCommsQueue ();
            }
        }
    }
}

void Espnow_halClass::runHandle (void* param) {
#ifdef ESP32
    for (;;) {
#endif
        Espnow_hal.handle ();
#ifdef ESP32
        vTaskDelay (1 / portTICK_PERIOD_MS);
    }
#endif
}
