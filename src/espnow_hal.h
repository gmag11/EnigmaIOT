/**
  * @file espnow_hal.h
  * @version 0.9.3
  * @date 14/07/2020
  * @author German Martin
  * @brief ESP-NOW communication system abstraction layer. To be used on ESP8266 or ESP32 platforms
  */

#ifndef _ESPNOW_HAL_h
#define _ESPNOW_HAL_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_now.h>
#endif
#include "Comms_hal.h"
#include "helperFunctions.h"
#include "debug.h"

/**
  * @brief Definition for ESP-NOW hardware abstraction layer
  */
class Espnow_halClass : public Comms_halClass {
public:
	static const size_t COMMS_HAL_MAX_MESSAGE_LENGTH = 250; ///< @brief Maximum message length for ESP-NOW
	static const uint8_t COMMS_HAL_ADDR_LEN = 6; ///< @brief Address length for ESP-NOW. Correspond to mac address

protected:

	uint8_t gateway[COMMS_HAL_ADDR_LEN]; ///< @brief Gateway address
	uint8_t channel; ///< @brief WiFi channel to be used

	comms_hal_rcvd_data dataRcvd; ///< @brief Pointer to a function to be called on every received message
	comms_hal_sent_data sentResult; ///< @brief Pointer to a function to be called to notify last sending status

	/**
	  * @brief Communication subsistem initialization
	  * @param peerType Role that peer plays into the system, sensor node or gateway.
	  */
	void initComms (peerType_t peerType);

	/**
	  * @brief Function that processes incoming messages and passes them to upper layer
	  * @param mac_addr Destination address to send the message to
	  * @param data Data buffer that contain the message to be sent
	  * @param len Data length in number of bytes
	  */
	static void ICACHE_FLASH_ATTR rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len);

	/**
	  * @brief Function that gets sending status
	  * @param mac_addr Destination address to send the message to
	  * @param status Sending status
	  */
	static void ICACHE_FLASH_ATTR tx_cb (uint8_t* mac_addr, uint8_t status);

public:
   /**
	 * @brief Setup communication environment and establish the connection from node to gateway
	 * @param gateway Address of gateway. It may be `NULL` in case this is used in the own gateway
	 * @param channel Establishes a channel for the communication. Its use depends on actual communications subsystem
	 * @param peerType Role that peer plays into the system, sensor node or gateway.
	 */
	void begin (uint8_t* gateway, uint8_t channel = 0, peerType_t peerType = COMM_NODE);

	/**
	 * @brief Terminates communication and closes all connectrions
	 */
	void stop ();

	/**
	  * @brief Sends data to the other peer
	  * @param da Destination address to send the message to
	  * @param data Data buffer that contain the message to be sent
	  * @param len Data length in number of bytes
	  * @return Returns sending status. 0 for success, 1 to indicate an error.
	  */
	int32_t send (uint8_t* da, uint8_t* data, int len);
	/**
	  * @brief Attach a callback function to be run on every received message
	  * @param dataRcvd Pointer to the callback function
	  */
	void onDataRcvd (comms_hal_rcvd_data dataRcvd);

	/**
	  * @brief Attach a callback function to be run after sending a message to receive its status
	  * @param dataRcvd Pointer to the callback function
	  */
	void onDataSent (comms_hal_sent_data dataRcvd);

	/**
	  * @brief Get address length used on ESP-NOW subsystem
	  * @return Always returns the sice of 802.11 MAC address, equals to 6
	  */
	uint8_t getAddressLength () {
		return COMMS_HAL_ADDR_LEN;
	}

	/**
	  * @brief Get maximum message length on ESP-NOW subsystem
	  * @return Always returns a value equal to 250
	  */
	size_t getMaxMessageLength () {
		return COMMS_HAL_MAX_MESSAGE_LENGTH;
	}

};

extern Espnow_halClass Espnow_hal; ///< @brief Singleton instance of ESP-NOW class

#endif

