// 
// 
// 

#include "Node.h"

#include "lib/helperFunctions.h"

void Node::setEncryptionKey (const uint8_t* key) {
    if (key) {
        memcpy (this->key, key, KEYLENGTH);
    }
}

node_t Node::getNodeData () {
    node_t thisNode;

    memcpy (thisNode.key, key, KEYLENGTH);
    thisNode.keyValid = keyValid;
    thisNode.keyValidFrom = keyValidFrom;
    memcpy (thisNode.mac, mac, 6);
    thisNode.nodeId = nodeId;
    thisNode.lastMessageCounter = lastMessageCounter;
    //thisNode.registered = registered;
    thisNode.status = status;
}

void Node::setMacAddress (uint8_t *mac) {
    if (mac) {
        memcpy (this->mac, mac, 6);
    }
}

void Node::setMacAddress (const uint8_t *macAddress) {
    if (macAddress) {
        memcpy (mac, macAddress, 6);
    }
}


void Node::printToSerial (Stream *port)
{
    port->println ();
    port->printf ("Node: %d\n", nodeId);
    char macstr[18];
    mac2str (mac, macstr);
    port->printf ("\tMAC Address: %s\n", macstr);
    port->printf ("\tLast counter: %u\n", lastMessageCounter);
    port->printf ("\tKey valid from: %u ms ago\n", millis () - keyValidFrom);
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
    case WAIT_FOR_KEY_EXCH_FINISHED:
        port->println ("Wait for Key Exchange Finished");
        break;
    case WAIT_FOR_CIPHER_FINISHED:
        port->println ("Wait for Cipher Finished");
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

void Node::reset () {
    memset (mac, 0, 6);
    memset (key, 0, KEYLENGTH);
    keyValid = false;
    lastMessageCounter = 0;
    keyValidFrom = 0;
    status = UNREGISTERED;
    sleepyNode = true;
}

Node::Node () :
    keyValid (false),
    status (UNREGISTERED)
    //registered (false)
{
}

Node::Node (node_t nodeData) :
    keyValid (nodeData.keyValid),
    keyValidFrom (nodeData.keyValidFrom),
    lastMessageCounter (nodeData.lastMessageCounter),
    nodeId (nodeData.nodeId),
    status (nodeData.status)
    //registered(nodeData.registered)
{
    memcpy (key, nodeData.key, sizeof (uint16_t));
    memcpy (mac, nodeData.mac, 6);
}

