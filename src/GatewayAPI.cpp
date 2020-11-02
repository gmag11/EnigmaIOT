#include "GatewayAPI.h"
#include <functional>

using namespace std;
using namespace placeholders;

const char* getNodeNumberUri = "/gw/api/nodenumber";
const char* getMaxNodesUri = "/gw/api/maxnodes";
const char* getNodesUri = "/gw/api/nodes";
const char* getNodeUri = "/gw/api/node";
const char* nodeIdParam = "nodeid";
const char* nodeNameParam = "nodename";
const char* nodeAddrParam = "nodeaddr";

void GatewayAPI::begin () {
	//if (!gw) {
	//	return;
	//}
	//gateway = gw;
	server = new AsyncWebServer (WEB_API_PORT);
	server->on (getNodeNumberUri, HTTP_GET, std::bind (&GatewayAPI::getNodeNumber, this, _1));
	server->on (getMaxNodesUri, HTTP_GET, std::bind (&GatewayAPI::getMaxNodes, this, _1));
	server->on (getNodesUri, HTTP_GET, std::bind (&GatewayAPI::getNodes, this, _1));
	server->on (getNodeUri, HTTP_GET | HTTP_DELETE, std::bind (&GatewayAPI::nodeOp, this, _1));
	server->onNotFound (std::bind (&GatewayAPI::onNotFound, this, _1));
	server->begin ();
}

void GatewayAPI::getNodeNumber (AsyncWebServerRequest* request) {
	char response[25];

	snprintf (response, 25, "{'nodeNumber':%d}", EnigmaIOTGateway.getActiveNodesNumber ());
	DEBUG_INFO ("Response: %s", response);
	request->send (200, "application/json", response);
}

Node* GatewayAPI::getNodeFromParam (AsyncWebServerRequest* request) {
	Node* node = NULL;
	int params = request->params ();
	int nodeIndex = -1;

	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam (i);
		if (p->name () == nodeIdParam) {
			if (isNumber (p->value ())) {
				nodeIndex = atoi (p->value ().c_str ());
				DEBUG_INFO ("Node to process is %d", nodeIndex);
				node = EnigmaIOTGateway.nodelist.getNodeFromID (nodeIndex);
				break;
			}
		}
		if (p->name () == nodeNameParam) {
			if (strcmp (p->value ().c_str (), BROADCAST_NONE_NAME)) {
				node = EnigmaIOTGateway.nodelist.getNodeFromName (p->value ().c_str ());
				DEBUG_INFO ("Node to process is %s", node ? node->getNodeName () : "NULL");
			} else {
				DEBUG_INFO ("Wrong node name %s", p->value ().c_str ());
			}
			break;
		}
		if (p->name () == nodeAddrParam) {
			uint8_t addr[ENIGMAIOT_ADDR_LEN];
			uint8_t* addrResult = str2mac (p->value ().c_str (), addr);
			if (addrResult) {
				if (memcmp (addr, BROADCAST_ADDRESS, ENIGMAIOT_ADDR_LEN)) {
					node = EnigmaIOTGateway.nodelist.getNodeFromMAC (addr);
					DEBUG_INFO ("Node to process is %s", addr ? p->value ().c_str () : "NULL");
				}
			}
			break;
		}
		DEBUG_DBG ("Parameter %s = %s", p->name ().c_str (), p->value ().c_str ());
	}

	DEBUG_DBG ("NodeId = %d, node: %p", nodeIndex, node);

	if (node) {
		if (node->isRegistered ()) {
			return node;
		}
	}
	return node;
}

const char* GatewayAPI::deleteNode (Node* node, int& resultCode) {
	if (node) {
		DEBUG_DBG ("Node %d is %p", node->getNodeId (), node);
		if (node->isRegistered ()) {
			DEBUG_INFO ("Node %d is registered", node->getNodeId ());
			resultCode = 200;
			EnigmaIOTGateway.invalidateKey (node, KICKED);
			return "{'result':'ok'}";
		} else {
			DEBUG_INFO ("Node %d is not registered", node->getNodeId ());
		}
	} 
	return NULL;
}

char* GatewayAPI::getNodeInfo (Node* node, int& resultCode, char* nodeInfo, size_t len) {
	if (node) {
		DEBUG_DBG ("Node %d is %p", node->getNodeId (), node);
		if (node->isRegistered ()) {
			DEBUG_INFO ("Node %d is registered", node->getNodeId ());
			resultCode = 200;
			time_t currentMillis = millis ();
			snprintf (nodeInfo, len, "{'node_id':%d,'address':'" MACSTR "',"\
				"'Name':'%s','keyValidSince':%d,'lastMessageTime':%d,'sleepy':%s,"\
				"'Broadcast':%s,'rssi':%d}",
					  node->getNodeId (),
					  MAC2STR (node->getMacAddress ()),
					  node->getNodeName (),
					  currentMillis-node->getKeyValidFrom(),
					  currentMillis-node->getLastMessageTime(),
					  node->getSleepy() ? "True" : "False",
					  node->broadcastIsEnabled() ? "True" : "False",
					  node->getRSSI()
			);
			DEBUG_DBG ("NodeInfo: %s", nodeInfo);
			return nodeInfo;
		} else {
			DEBUG_INFO ("Node %d is not registered", node->getNodeId ());
		}
	}
	return NULL;
}

void GatewayAPI::nodeOp (AsyncWebServerRequest* request) {
	Node* node;
	int resultCode = 404;
	char response[RESPONSE_SIZE];

	node = getNodeFromParam (request);

	WebRequestMethodComposite method = request->method ();
	DEBUG_INFO ("Method: %s", methodToString (request->method ()).c_str ());
	
	if (method == HTTP_DELETE){
		DEBUG_INFO ("Delete node %p", node);
		const char* strTemp = deleteNode (node, resultCode);
		if (strTemp) {
			strncpy (response, strTemp, RESPONSE_SIZE);
		}
	} else if (method == HTTP_GET) {
		DEBUG_INFO ("Info node %p", node);
		const char* strTemp = getNodeInfo (node, resultCode, response, RESPONSE_SIZE);
		DEBUG_DBG ("strTemp = %p", strTemp);
		if (!strTemp) {
			//strncpy (response, strTemp, 200);
			resultCode = 404;
		}
	}
	if (resultCode == 404) {
		snprintf (response, 25, "{'result':'not found'}");
	}
	DEBUG_DBG ("Response: %d --> %s", resultCode, response);
	request->send (resultCode, "application/json", response);
}

void GatewayAPI::getMaxNodes (AsyncWebServerRequest* request) {
	char response[25];

	snprintf (response, 25, "{'maxNodes':%d}", NUM_NODES);
	DEBUG_INFO ("Response: %s", response);
	request->send (200, "application/json", response);
}

void GatewayAPI::getNodes (AsyncWebServerRequest* request) {
	Node* node = NULL;

	AsyncResponseStream* response = request->beginResponseStream ("application/json");
	response->setCode (200);

	response->print ("{'nodes':[");
	do {
		bool first = (node == NULL);
		node = EnigmaIOTGateway.nodelist.getNextActiveNode (node);
		if (node && !first) {
			DEBUG_DBG ("First is %s, node is %p", first ? "true" : "false", node);
			response->print (',');
		}
		if (node) {
			DEBUG_DBG ("LastNode: %u, node: %p", node->getNodeId (), node);
		}
		if (node) {
			DEBUG_DBG ("Got node. NodeId -> %u", node->getNodeId ());
			response->printf ("{'nodeId':%u,'address':'" MACSTR "','name':'%s'}",
							  node->getNodeId (),
							  MAC2STR (node->getMacAddress ()),
							  node->getNodeName ());
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
	DEBUG_WARN ("404 Not found: %s", request->url ().c_str ());
	DEBUG_WARN ("Method: %s", methodToString (request->method ()).c_str ());
	int params = request->params ();
	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam (i);
		DEBUG_INFO ("Parameter %s = %s", p->name ().c_str (), p->value ().c_str ());
	}
	request->send (404, "text/plain", "Not Found");
}

GatewayAPI GwAPI;