/**
  * @file GwOutput_generic.h
  * @version 0.6.0
  * @date 24/11/2019
  * @author German Martin
  * @brief Generic Gateway output module template
  *
  * This is the interface that output module should implement to be used as Gateway Output
  */

#ifndef _GWOUT_GEN_h
#define _GWOUT_GEN_h

#include <ESPAsyncWiFiManager.h>
#include <EnigmaIOTGateway.h>
#include <lib/helperFunctions.h>
#include <lib/debug.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class GatewayOutput_generic {
protected:
	EnigmaIOTGatewayClass* enigmaIotGateway;

public:
	String netName;
	String clientId;
	String gwTopic;

	//virtual int send () = 0;
	//virtual void onReveive () = 0;
	virtual void configManagerStart (EnigmaIOTGatewayClass* enigmaIotGw) = 0;
	virtual void configManagerExit (boolean status) = 0;
	virtual bool begin () = 0;
	virtual bool loadConfig () = 0;
	virtual bool outputSend (char* address, uint8_t* data, uint8_t length) = 0;
};

#endif // _GWOUT_GEN_h