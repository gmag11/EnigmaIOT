/**
  * @file GatewayAPI.h
  * @version 0.9.7
  * @date 17/12/2020
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

const size_t RESPONSE_SIZE = 250;  ///< @brief Maximum API response size

String methodToString (WebRequestMethodComposite method);

class GatewayAPI {
protected:
	AsyncWebServer* server; ///< @brief Web server instance
	//EnigmaIOTGatewayClass* gateway;

    /**
     * @brief Processes node number request
     * @param request Node number request
     */
	void getNodeNumber (AsyncWebServerRequest* request);
    
    /**
     * @brief Processes max node number request
     * @param request Max node number request
     */
	void getMaxNodes (AsyncWebServerRequest* request);
    
    /**
     * @brief Processes node list request
     * @param request Node list request
     */
	void getNodes (AsyncWebServerRequest* request);
    
    /**
     * @brief Processes node information request
     * @param request Node information request
     */
	void nodeOp (AsyncWebServerRequest* request);
    
    /**
     * @brief Processes gateway information request
     * @param request Gateway information request
     */
	void getGwInfo (AsyncWebServerRequest* request);
    
    /**
     * @brief Processes gateway restart request
     * @param request Gateway restart request
     */
	void restartGw (AsyncWebServerRequest* request);
    
    /**
     * @brief Processes node information request
     * @param request Node information request
     */
	void restartNode (AsyncWebServerRequest* request);
	// TODO: Reset node
	// TODO: Reset Gw

    /**
     * @brief Processes unknown entry points or methods
     * @param request Request
     */
	void onNotFound (AsyncWebServerRequest* request);

    /**
     * @brief Gets node reference from request parameters
     * @param request Request with node parameter (NodeID, Name or MAC address)
     */
	Node* getNodeFromParam (AsyncWebServerRequest* request);

    /**
     * @brief Processes node deletion request
     * @param node Node to delete
     * @param resultCode Result code
     */
	const char* deleteNode (Node* node, int& resultCode);
    
    /**
     * @brief Builds node info
     * @param node Node to get info from
     * @param resultCode Result code
     * @param nodeInfo Node information JSON element
     * @param len JSON length
     */
	char* getNodeInfo (Node* node, int& resultCode, char* nodeInfo, size_t len);
    
    /**
     * @brief Builds gateway info
     * @param gwInfo Gateway information JSON
     * @param len JSON length
     */
	char* buildGwInfo (char* gwInfo, size_t len);
     
    /**
     * @brief Sends restart node message
     * @param node Node te send restart to
     */    
	bool restartNodeRequest (Node* node);

public:
    
    /**
     * @brief Starts REST API web server
     */
	void begin ();
};

extern GatewayAPI GwAPI; ///< @brief API instance

#endif // GATEWAY_API_h