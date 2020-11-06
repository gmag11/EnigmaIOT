#include "GatewayAPI.h"
#include <functional>

using namespace std;
using namespace placeholders;

const char* getNodeNumberUri = "/api/gw/nodenumber";
const char* getMaxNodesUri = "/api/gw/maxnodes";
const char* getNodesUri = "/api/gw/nodes";
const char* getNodeUri = "/api/node/node";
const char* getGwInfoUri = "/api/gw/info";
const char* getGwRestartUri = "/api/gw/restart";
const char* getNodeRestartUri = "/api/node/restart";
const char* nodeIdParam = "nodeid";
const char* nodeNameParam = "nodename";
const char* nodeAddrParam = "nodeaddr";
const char* confirmParam = "confirm";

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
	server->on (getGwInfoUri, HTTP_GET, std::bind (&GatewayAPI::getGwInfo, this, _1));
	server->on (getGwRestartUri, HTTP_PUT, std::bind (&GatewayAPI::restartGw, this, _1));
	server->on (getNodeRestartUri, HTTP_PUT, std::bind (&GatewayAPI::restartNode, this, _1));
	server->onNotFound (std::bind (&GatewayAPI::onNotFound, this, _1));
	server->begin ();
}

void GatewayAPI::getNodeNumber (AsyncWebServerRequest* request) {
	char response[25];

	snprintf (response, 25, "{'nodeNumber':%d}", EnigmaIOTGateway.getActiveNodesNumber ());
	DEBUG_INFO ("Response: %s", response);
	request->send (200, "application/json", response);
}

char* GatewayAPI::buildGwInfo (char* gwInfo, size_t len) {
	DEBUG_INFO ("Build Gateway Info");
	//resultCode = 200;
	//time_t currentMillis = millis ();
	snprintf (gwInfo, len, "{'version':'%s','network':'%s','addresses':{'AP':'%s','STA':'%s'},"
			  "'channel':%d,'ap':'%s','bssid':'%s','rssi':%d,"
#ifdef ESP32
			  "txpower':%.1f,"
#endif
			  "'dns':'%s'}",
			  ENIGMAIOT_PROT_VERS, EnigmaIOTGateway.getNetworkName (),
			  WiFi.macAddress ().c_str (), WiFi.softAPmacAddress ().c_str (),
			  WiFi.channel (), WiFi.SSID ().c_str(), WiFi.BSSIDstr ().c_str(), WiFi.RSSI(),
#ifdef ESP32
			  (float)(WiFi.getTxPower ())/4, 
#endif
			  WiFi.dnsIP ().toString ().c_str ()
	);
	DEBUG_DBG ("GwInfo: %s", gwInfo);
	return gwInfo;
}

void GatewayAPI::getGwInfo (AsyncWebServerRequest* request) {
	int resultCode = 404;
	char response[RESPONSE_SIZE];
	const char* strTemp = buildGwInfo (response, RESPONSE_SIZE);
	if (strTemp) {
		resultCode = 200;
	}
	if (resultCode == 404) {
		snprintf (response, RESPONSE_SIZE, "{'result':'not found'}");
	}
	DEBUG_DBG ("Response: %d --> %s", resultCode, response);
	request->send (resultCode, "application/json", response);
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
				"'Broadcast':%s,'rssi':%d,'packetsHour':%f,'per':%f}",
					  node->getNodeId (),
					  MAC2STR (node->getMacAddress ()),
					  node->getNodeName (),
					  currentMillis-node->getKeyValidFrom(),
					  currentMillis-node->getLastMessageTime(),
					  node->getSleepy() ? "True" : "False",
					  node->broadcastIsEnabled() ? "True" : "False",
					  node->getRSSI(),
					  node->packetsHour,
					  node->per
			);
			DEBUG_DBG ("NodeInfo: %s", nodeInfo);
			return nodeInfo;
		} else {
			DEBUG_INFO ("Node %d is not registered", node->getNodeId ());
		}
	}
	return NULL;
}

bool GatewayAPI::restartNodeRequest (Node* node) {
	return EnigmaIOTGateway.sendDownstream (node->getMacAddress (), NULL, 0, RESTART_NODE);
}

void GatewayAPI::restartNode (AsyncWebServerRequest* request) {
	Node* node;
	int resultCode = 404;
	char response[RESPONSE_SIZE];

	node = getNodeFromParam (request);

	DEBUG_WARN ("Send restart command to node %p", node);

	bool result = restartNodeRequest (node);
	if (result) {
		snprintf (response, 30, "{'node_restart':'processed'}");
		resultCode = 200;
	}
	if (resultCode == 404) {
		snprintf (response, 25, "{'result':'not found'}");
	}
	DEBUG_WARN ("Response: %d --> %s", resultCode, response);
	request->send (resultCode, "application/json", response);
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

void GatewayAPI::restartGw (AsyncWebServerRequest* request) {
	char response[30];
	bool confirm = false;
	int resultCode = 404;
	
	int params = request->params ();

	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam (i);
		if (p->name () == confirmParam) {
			if (p->value () == "1") {
				confirm = true;
				resultCode = 200;
				break;
			}
		}
	}

	if (confirm) {
		snprintf (response, 30, "{'gw_restart':'processed'}");
		request->send (resultCode, "application/json", response);
	} else {
		snprintf (response, 25, "{'gw_restart':'fail'}");
		request->send (resultCode, "application/json", response);
	}

	DEBUG_INFO ("Response: %s", response);
	
	if (confirm && EnigmaIOTGateway.notifyRestartRequested) {
		EnigmaIOTGateway.notifyRestartRequested();
	}
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