/**
  * @file GatewayAPI.h
  * @version 0.9.5
  * @date 30/10/2020
  * @author German Martin
  * @brief API web server to control EnigmaIOT Gateway
  */

#ifndef GATEWAY_API_h
#define GATEWAY_API_h

#include <EnigmaIOTGateway.h>
#include <ESPAsyncWebServer.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class GatewayAPI {
protected:
	AsyncWebServer* server;
	//EnigmaIOTGatewayClass* gateway;

public:
	void begin ();
};

extern GatewayAPI GwAPI;

#endif // GATEWAY_API_h