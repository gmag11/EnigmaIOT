/**
  * @file GwOutput_generic.h
  * @version 0.9.1
  * @date 28/05/2020
  * @author German Martin
  * @brief Generic Gateway output module template
  *
  * This is the interface that output module should implement to be used as Gateway Output
  */

#ifndef _GWOUT_GEN_h
#define _GWOUT_GEN_h

#include <EnigmaIOTGateway.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

typedef enum GwOutput_data_type {
	data,
	lostmessages,
	status
} GwOutput_data_type_t;

#include <functional>
typedef std::function<void (uint8_t* address, control_message_type_t msgType, char* data, unsigned int len)> onDlData_t;

class GatewayOutput_generic {
protected:
	EnigmaIOTGatewayClass* enigmaIotGateway; ///< @brief Pointer to EnigmaIOT gateway instance
	onDlData_t downlinkCb; ///< @brief downlink processing function handle

	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	virtual bool saveConfig () = 0;

public:
	String netName; ///< @brief EnigmaIOT network name
	String clientId; ///< @brief MQTT clientId
	String gwTopic; ///< @brief MQTT topic for gateway

	//virtual int send () = 0;
	//virtual void onReveive () = 0;

	/**
	  * @brief Called when wifi manager starts config portal
	  * @param enigmaIotGw Pointer to EnigmaIOT gateway instance
	  */
	virtual void configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw) = 0;

	/**
	  * @brief Called when wifi manager exits config portal
	  * @param status `true` if configuration was successful
	  */
	virtual void configManagerExit (bool status) = 0;

	/**
	  * @brief Starts output module
	  * @return Returns `true` if successful. `false` otherwise
	  */
	virtual bool begin () = 0;

	/**
	  * @brief Loads output module configuration
	  * @return Returns `true` if load was successful. `false` otherwise
	  */
	virtual bool loadConfig () = 0;

	 /**
	  * @brief Send control data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	virtual bool outputControlSend (char* address, uint8_t* data, size_t length) = 0;

	 /**
	  * @brief Send new node notification
	  * @param address Node Address
	  * @param node_id Node Id
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	virtual bool newNodeSend (char* address, uint16_t node_id) = 0;

	 /**
	  * @brief Send node disconnection notification
	  * @param address Node Address
	  * @param reason Disconnection reason code
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	virtual bool nodeDisconnectedSend (char* address, gwInvalidateReason_t reason) = 0;

	 /**
	  * @brief Send data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @param type Type of message
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	virtual bool outputDataSend (char* address, char* data, size_t length, GwOutput_data_type_t type = data) = 0;

	 /**
	  * @brief Should be called often for module management
	  */
	virtual void loop () = 0;

	 /**
	  * @brief Set data processing function
	  * @param cb Function handle
	  */
	void setDlCallback (onDlData_t cb) {
		downlinkCb = cb;
	}
};

#endif // _GWOUT_GEN_h