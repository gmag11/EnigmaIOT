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
    thisNode.lastMessageTime = lastMessageTime;
    memcpy (thisNode.mac, mac, 6);
    thisNode.nodeId = nodeId;
    thisNode.lastMessageCounter = lastMessageCounter;
    //thisNode.registered = registered;
    thisNode.status = status;
}


void Node::printToSerial (Stream *port)
{
    port->println ();
    port->printf ("Node: %d\n", nodeId);
    char macstr[18];
    mac2str (mac, macstr);
    port->printf ("\tMAC Address: %s\n", macstr);
    port->printf ("\tLast counter: %u\n", lastMessageCounter);
    port->printf ("\tLast message: %u ms ago\n", millis () - lastMessageTime);
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
    lastMessageTime = 0;
    status = UNREGISTERED;
}

Node::Node () :
    keyValid (false),
    status (UNREGISTERED)
    //registered (false)
{
}

Node::Node (node_t nodeData) :
    keyValid (nodeData.keyValid),
    lastMessageTime (nodeData.lastMessageTime),
    lastMessageCounter (nodeData.lastMessageCounter),
    nodeId (nodeData.nodeId),
    status (nodeData.status)
    //registered(nodeData.registered)
{
    memcpy (key, nodeData.key, sizeof (uint16_t));
    memcpy (mac, nodeData.mac, 6);
}

