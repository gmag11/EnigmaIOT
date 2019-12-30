/**
  * @file GwOutput_generic.h
  * @version 0.7.0
  * @date 30/12/2019
  * @author German Martin
  * @brief Generic Gateway output module template
  *
  * This is the interface that output module should implement to be used as Gateway Output
  */

#ifndef _GWOUT_GEN_h
#define _GWOUT_GEN_h

#include <Arduino.h>
#include <EnigmaIOTGateway.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
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
	EnigmaIOTGatewayClass* enigmaIotGateway;
	onDlData_t downlinkCb;
	virtual bool saveConfig () = 0;

public:
	String netName;
	String clientId;
	String gwTopic;

	//virtual int send () = 0;
	//virtual void onReveive () = 0;

	virtual void configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw) = 0;
	virtual void configManagerExit (bool status) = 0;
	virtual bool begin () = 0;
	virtual bool loadConfig () = 0;
	virtual bool outputControlSend (char* address, uint8_t* data, uint8_t length) = 0;
	virtual bool newNodeSend (char* address) = 0;
	virtual bool nodeDisconnectedSend (char* address, gwInvalidateReason_t reason) = 0;
	virtual bool outputDataSend (char* address, char* data, uint8_t length, GwOutput_data_type_t type = data) = 0;
	virtual void loop () = 0;
	void setDlCallback (onDlData_t cb) {
		downlinkCb = cb;
	}
};

#endif // _GWOUT_GEN_h