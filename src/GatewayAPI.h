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

const size_t RESPONSE_SIZE = 200;

String methodToString (WebRequestMethodComposite method);

class GatewayAPI {
protected:
	AsyncWebServer* server;
	//EnigmaIOTGatewayClass* gateway;

	void getNodeNumber (AsyncWebServerRequest* request);
	void getMaxNodes (AsyncWebServerRequest* request);
	void getNodes (AsyncWebServerRequest* request);
	void nodeOp (AsyncWebServerRequest* request);

	void onNotFound (AsyncWebServerRequest* request);

	Node* getNodeFromParam (AsyncWebServerRequest* request);

	const char* deleteNode (Node* node, int& resultCode);
	char* getNodeInfo (Node* node, int& resultCode, char* nodeInfo, size_t len);

public:
	void begin ();
};

extern GatewayAPI GwAPI;

#endif // GATEWAY_API_h