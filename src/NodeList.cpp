/**
  * @file NodeList.cpp
  * @version 0.9.6
  * @date 10/12/2020
  * @author German Martin
  * @brief EnigmaIoT sensor node management structures
  */
#include "NodeList.h"
#include "helperFunctions.h"

void Node::setEncryptionKey (const uint8_t* key) {
	if (key) {
		memcpy (this->key, key, KEY_LENGTH);
	}
}

node_t Node::getNodeData () {
	node_t thisNode;

	memcpy (thisNode.key, key, KEY_LENGTH);
	thisNode.keyValid = keyValid;
	thisNode.keyValidFrom = keyValidFrom;
	memcpy (thisNode.mac, mac, 6);
	thisNode.nodeId = nodeId;
	thisNode.lastMessageCounter = lastMessageCounter;
	thisNode.status = status;
	memcpy (thisNode.nodeName, getNodeName (), NODE_NAME_LENGTH);

	return thisNode;
}

void Node::printToSerial (Stream* port) {
	port->println ();
	port->printf ("Node: %d\n", nodeId);
	char macstr[ENIGMAIOT_ADDR_LEN * 3];
	mac2str (mac, macstr);
	port->printf ("\tMAC Address: %s\n", macstr);
	port->printf ("\tLast counter: %u\n", lastMessageCounter);
	port->printf ("\tKey valid from: %lu ms ago\n", (millis () - keyValidFrom));
	port->printf ("\tKey: %s\n", keyValid ? "Valid" : "Invalid");
	port->print ("\tStatus: ");
	switch (status) {
	case UNREGISTERED:
		port->println ("Unregistered");
		break;
	case INIT:
		port->println ("Initializing");
		break;
	case SLEEP:
		port->println ("Going to sleep");
		break;
	case WAIT_FOR_SERVER_HELLO:
		port->println ("Wait for server hello");
		break;
	case WAIT_FOR_DOWNLINK:
		port->println ("Wait for Downlik");
		break;
	case REGISTERED:
		port->println ("Registered. Wait for messages");
		break;
	default:
		port->println (status);
	}
	port->println ();
}

void Node::initRateFilter () {
	float weight = 1;

	rateFilter = new FilterClass (AVERAGE_FILTER, RATE_AVE_ORDER);
	for (int i = 0; i < RATE_AVE_ORDER; i++) {
		rateFilter->addWeigth (weight);
		weight = weight / 2;
	}

}

Node::Node () :
	keyValid (false),
	status (UNREGISTERED) {
	initRateFilter ();
}

Node::Node (node_t nodeData) :
	keyValid (nodeData.keyValid),
	status (nodeData.status),
	lastMessageCounter (nodeData.lastMessageCounter),
	nodeId (nodeData.nodeId),
	keyValidFrom (nodeData.keyValidFrom),
	sleepyNode (nodeData.sleepyNode)
	//packetNumber (0),
	//packetErrors (0),
	//per (0.0)
{
	memcpy (key, nodeData.key, sizeof (uint16_t));
	memcpy (mac, nodeData.mac, 6);

	initRateFilter ();
}

void Node::updatePacketsRate (float value) {
	packetsHour = rateFilter->addValue (value);
}


void Node::reset () {
	DEBUG_DBG ("Reset node");
	//memset (mac, 0, 6);
	memset (key, 0, KEY_LENGTH);
	memset (nodeName, 0, NODE_NAME_LENGTH);
	keyValid = false;
	lastMessageCounter = 0;
	lastControlCounter = 0;
	lastDownlinkMsgCounter = 0;
	keyValidFrom = 0;
	status = UNREGISTERED;
	enigmaIOTVersion[0] = 0;
	enigmaIOTVersion[1] = 0;
	enigmaIOTVersion[2] = 0;
	//broadcastEnabled = false;
	broadcastKeyRequested = false;
	if (rateFilter) {
		DEBUG_DBG ("Reset packet rate");
		rateFilter->clear ();
	}
	//sleepyNode = true;
}

NodeList::NodeList () {
	for (int i = 0; i < NUM_NODES; i++) {
		nodes[i].nodeId = i;
	}
}

Node* NodeList::getNodeFromID (uint16_t nodeId) {
	if (nodeId >= NUM_NODES)
		return NULL;

	return &(nodes[nodeId]);
}

Node* NodeList::getNodeFromMAC (const uint8_t* mac) {
	uint16_t index = 0;

	if (!memcmp (broadcastNode->getEncriptionKey (), mac, ENIGMAIOT_ADDR_LEN)) {
		return broadcastNode;
	}

	while (index < NUM_NODES) {
		if (!memcmp (nodes[index].mac, mac, ENIGMAIOT_ADDR_LEN)) {
			if (nodes[index].status != UNREGISTERED) {
				return &(nodes[index]);
			}
		}
		index++;
	}

	return NULL;
}

void NodeList::initBroadcastNode () {
	broadcastNode = new Node ();
	node_t node;

	//uint8_t broadcastAddress[ENIGMAIOT_ADDR_LEN];
	//memcpy (broadcastAddress, BROADCAST_ADDRESS, ENIGMAIOT_ADDR_LEN);
	broadcastNode->setMacAddress (BROADCAST_ADDRESS);
	broadcastNode->setNodeId (0xffff);
	broadcastNode->setStatus (REGISTERED);
	broadcastNode->setSleepy (false);
	broadcastNode->setNodeName (BROADCAST_NONE_NAME);
}

Node* NodeList::getNodeFromName (const char* name) {
	uint16_t index = 0;

	// Check if address is an address as an string
	uint8_t netAddr[ENIGMAIOT_ADDR_LEN];
	if (str2mac (name, netAddr)) {
		return getNodeFromMAC (netAddr);
	}

	// check if destination is broadcast
	if (!strncmp (name, broadcastNode->getNodeName (), NODE_NAME_LENGTH)) {
		DEBUG_DBG ("Address '%s' is broadcast node", name);
		return broadcastNode;
	}

	while (index < NUM_NODES) {
		if (!strncmp (nodes[index].nodeName, name, NODE_NAME_LENGTH)) {
			if (nodes[index].status != UNREGISTERED) {
				return &(nodes[index]);
			}
		}
		index++;
	}

	return NULL;
}

int8_t NodeList::checkNodeName (const char* name, const uint8_t* address) {
	//bool found = false;

	if (strlen (name) > NODE_NAME_LENGTH - 1) {
		DEBUG_ERROR ("Name too long %s", name);
		return TOO_LONG; // Enmpty name
	}

	if (!strlen (name)) {
		DEBUG_ERROR ("Empty name", name);
		return EMPTY_NAME; // Too long name
	}

	for (int i = 0; i < NUM_NODES; i++) {
		// if node is not registered and has this node name
		DEBUG_DBG ("Node %d status is %d", i, nodes[i].status);
		if (nodes[i].status != UNREGISTERED) {
			char* currentNodeNamme = nodes[i].getNodeName ();
			DEBUG_DBG ("Node %d name is %s", i, currentNodeNamme ? currentNodeNamme : "NULL");
			if (currentNodeNamme && !strncmp (currentNodeNamme, name, NODE_NAME_LENGTH)) {
				// if addresses addresses are different
				//char addrStr[ENIGMAIOT_ADDR_LEN * 3];
				DEBUG_INFO ("Found node name %s in Node List with address %s", name, mac2str (address));
				if (memcmp (nodes[i].getMacAddress (), address, ENIGMAIOT_ADDR_LEN)) {
					DEBUG_ERROR ("Duplicated name %s", name);
					return ALREADY_USED; // Already used
				}
			}
		}
	}
	return NAME_OK; // Name was not used
}

Node* NodeList::findEmptyNode () {
	uint16_t index = 0;

	while (index < NUM_NODES) {
		if (nodes[index].status != UNREGISTERED) {
			return &(nodes[index]);
		}
		index++;
	}

	return NULL;
}

uint16_t NodeList::countActiveNodes () {
	uint16_t counter = 0;

	for (int i = 0; i < NUM_NODES; i++) {
		if (nodes[i].status != UNREGISTERED) {
			counter++;
		}
	}
	return counter;
}

bool NodeList::unregisterNode (uint16_t nodeId) {
	if (nodeId < NUM_NODES) {
		nodes[nodeId].reset ();

		if (nodes[nodeId].status != UNREGISTERED) {
			nodes[nodeId].status = UNREGISTERED;
			return true;
		}
	}
	return false;
}

bool NodeList::unregisterNode (const uint8_t* mac) {
	Node* node = getNodeFromMAC (mac);
	if (node) {
		node->reset ();
		node->status = UNREGISTERED;
		return true;
	} else {
		return false;
	}
}

bool NodeList::unregisterNode (Node* node) {
	if (node) {
		node->reset ();

		if (nodes[node->nodeId].status != UNREGISTERED) {
			nodes[node->nodeId].status = UNREGISTERED;
			return true;
		} else {
			return false;
		}
	}
	return false;
}

Node* NodeList::getNextActiveNode (uint16_t nodeId) {
	if (nodeId == 0xFFFF) {
		if (nodes[0].status != UNREGISTERED) {
			return &(nodes[0]);
		}
	}
	for (int i = nodeId + 1; i < NUM_NODES; i++) {
		if (nodes[i].status != UNREGISTERED) {
			return &(nodes[i]);
		}
	}
	return NULL;
}

Node* NodeList::getNextActiveNode (Node* node) {
	if (!node) {
		if (nodes[0].status != UNREGISTERED) {
			return &(nodes[0]);
		}
		node = &(nodes[0]);
	}
	for (int i = node->nodeId + 1; i < NUM_NODES; i++) {
		if (nodes[i].status != UNREGISTERED) {
			return &(nodes[i]);
		}
	}
	return NULL;
}

Node* NodeList::getNewNode (const uint8_t* mac) {
	Node* node = getNodeFromMAC (mac);
	if (node) {
		return node;
	} else {
		for (int i = 0; i < NUM_NODES; i++) {
			if (nodes[i].status == UNREGISTERED) {
				nodes[i].setMacAddress (const_cast<uint8_t*>(mac));
				nodes[i].reset ();
				return &(nodes[i]);
			}
		}
	}
	return NULL;
}

void NodeList::printToSerial (Stream* port) {
	for (int i = 0; i < NUM_NODES; i++) {
		if (nodes[i].status != UNREGISTERED) {
			nodes[i].printToSerial (port);
		}
	}
}
