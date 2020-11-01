#include "GatewayAPI.h"
#include <functional>

using namespace std;
using namespace placeholders;

const char* getNodeNumberUri = "/gw/api/nodenumber";
const char* getMaxNodesUri = "/gw/api/maxnodes";
const char* getNodesUri = "/gw/api/nodes";

void GatewayAPI::begin () {
	//if (!gw) {
	//	return;
	//}
	//gateway = gw;
	server = new AsyncWebServer (WEB_API_PORT);
	server->on (getNodeNumberUri, HTTP_GET, std::bind (&GatewayAPI::getNodeNumber, this, _1));
	server->on (getMaxNodesUri, HTTP_GET, std::bind (&GatewayAPI::getMaxNodes, this, _1));
	server->on (getNodesUri, HTTP_GET, std::bind (&GatewayAPI::getNodes, this, _1));
	server->onNotFound (std::bind (&GatewayAPI::onNotFound, this, _1));
	server->begin ();
}

void GatewayAPI::getNodeNumber (AsyncWebServerRequest* request) {
	char response[25];

	snprintf (response, 25,"{'nodeNumber':%d}", EnigmaIOTGateway.getActiveNodesNumber());
	DEBUG_WARN ("Response: %s", response);
	request->send (200, "application/json", response);
}

void GatewayAPI::getMaxNodes (AsyncWebServerRequest* request) {
	char response[25];

	snprintf (response, 25,"{'maxNodes':%d}", NUM_NODES);
	DEBUG_WARN ("Response: %s", response);
	request->send (200, "application/json", response);
}

void GatewayAPI::getNodes (AsyncWebServerRequest* request) {
	Node* node = NULL;
	uint16_t lastNode = -1;

	AsyncResponseStream* response = request->beginResponseStream ("application/json");
	response->setCode (200);

	response->print ("{'nodes':[");
	do {
		bool first = (node == NULL);
		node = EnigmaIOTGateway.nodelist.getNextActiveNode (lastNode);
		if (node && !first) {
			DEBUG_WARN ("First is %s, node is %p", first ? "true" : "false", node);
			response->print (',');
		}
		DEBUG_WARN ("LastNode: %u, node: %p", lastNode, node);
		if (node) {
			node_t nodeInfo = node->getNodeData ();
			lastNode = nodeInfo.nodeId;
			DEBUG_WARN ("Got node. NodeId -> %u", nodeInfo.nodeId);
			response->printf ("{'nodeId':%u,'address':" MACSTR ",'name':%s}", nodeInfo.nodeId,
							  MAC2STR (nodeInfo.mac), nodeInfo.nodeName);
		}
	} while (node != NULL);
	response->print ("]}");
	request->send (response);
}

String methodToString (WebRequestMethodComposite method) {
	switch (method) {
	case HTTP_GET:
		return String ("GET");
	case HTTP_POST:
		return String ("POST");
	case HTTP_DELETE:
		return String ("DELETE");
	case HTTP_PUT:
		return String ("PUT");
	case HTTP_PATCH:
		return String ("PATCH");
	case HTTP_HEAD:
		return String ("HEAD");
	case HTTP_OPTIONS:
		return String ("OPTIONS");

	}
}

void GatewayAPI::onNotFound (AsyncWebServerRequest* request) {
	DEBUG_WARN ("404 Not found: %s", request->url ().c_str());
	DEBUG_WARN ("Method: %s", methodToString(request->method ()).c_str());
	int params = request->params ();
	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam (i);
		DEBUG_WARN ("Parameter %s = %s", p->name ().c_str (), p->value ().c_str ());
	}
	request->send (404, "text/plain", "Not Found");
}

GatewayAPI GwAPI;