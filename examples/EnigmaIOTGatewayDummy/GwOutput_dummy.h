/**
  * @file GwOutput_generic.h
  * @version 0.8.1
  * @date 17/01/2020
  * @author German Martin
  * @brief  Dummy Gateway output module
  */

#ifndef _GWOUT_DUMMY_h
#define _GWOUT_DUMMY_h

#include <Arduino.h>
#include <EnigmaIOTGateway.h>
#include <GwOutput_generic.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <functional>
typedef std::function<void (uint8_t* address, control_message_type_t msgType, char* data, unsigned int len)> onDlData_t;

class GatewayOutput_dummy : public GatewayOutput_generic {
protected:
	EnigmaIOTGatewayClass* enigmaIotGateway; ///< @brief Pointer to EnigmaIOT gateway instance
	onDlData_t downlinkCb; ///< @brief downlink processing function handle

	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	bool saveConfig ();

public:
	String netName; ///< @brief EnigmaIOT network name
	String clientId; ///< @brief clientId
	String gwTopic; ///< @brief topic for gateway

	//virtual int send () = 0;
	//virtual void onReveive () = 0;

	/**
	  * @brief Called when wifi manager starts config portal
	  * @param enigmaIotGw Pointer to EnigmaIOT gateway instance
	  */
	void configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw);

	/**
	  * @brief Called when wifi manager exits config portal
	  * @param status `true` if configuration was successful
	  */
	void configManagerExit (bool status);

	/**
	  * @brief Starts output module
	  * @return Returns `true` if successful. `false` otherwise
	  */
	bool begin ();

	/**
	  * @brief Loads output module configuration
	  * @return Returns `true` if load was successful. `false` otherwise
	  */
	bool loadConfig ();

	 /**
	  * @brief Send control data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool outputControlSend (char* address, uint8_t* data, uint8_t length);

	 /**
	  * @brief Send new node notification
	  * @param address Node Address
	  * @param node_id Node Id
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool newNodeSend (char* address, uint16_t node_id);

	 /**
	  * @brief Send node disconnection notification
	  * @param address Node Address
	  * @param reason Disconnection reason code
	  * @param node_id Node Id
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool nodeDisconnectedSend (char* address, gwInvalidateReason_t reason);

	 /**
	  * @brief Send data from nodes
	  * @param address Node Address
	  * @param data Message data buffer
	  * @param length Data buffer length
	  * @param type Type of message
	  * @return Returns `true` if sending was successful. `false` otherwise
	  */
	bool outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type = data);

	 /**
	  * @brief Should be called often for module management
	  */
	void loop ();

	 /**
	  * @brief Set data processing function
	  * @param cb Function handle
	  */
	void setDlCallback (onDlData_t cb) {
		downlinkCb = cb;
	}
};

extern GatewayOutput_dummy GwOutput;

#endif // _GWOUT_DUMMY_h