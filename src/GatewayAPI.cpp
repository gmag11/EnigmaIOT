#include "GatewayAPI.h"

void GatewayAPI::begin () {
	//if (!gw) {
	//	return;
	//}
	//gateway = gw;
	server = new AsyncWebServer (WEB_API_PORT);
	EnigmaIOTGateway.getNetworkName ();
}

GatewayAPI GwAPI;