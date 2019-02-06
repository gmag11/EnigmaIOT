// 
// 
// 

#include "NodeList.h"

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

String Node::toString ()
{
    String nodeString = "Node: ";
    nodeString += nodeId;
    nodeString += "\n\tMAC Address: ";
    char macstr[18];
    nodeString += mac2str (mac,macstr);
    nodeString += "\n\tLast counter: ";
    nodeString += lastMessageCounter;
    nodeString += "\n\tLast message: ";
    nodeString += (millis()-lastMessageTime)/1000;
    nodeString += "\n\tKey: ";
    nodeString += keyValid ? "Valid" : "Invalid";
    nodeString += "ms ago\n\tStatus: ";
    switch (status) {
    case UNREGISTERED :
        nodeString += "Unregistered";
        break;
    case INIT:
        nodeString += "Initializing";
        break;
    case SLEEP:
        nodeString += "Going to sleep";
        break;
    case WAIT_FOR_SERVER_HELLO:
        nodeString += "Wait for server hello";
        break;
    case WAIT_FOR_KEY_EXCH_FINISHED:
        nodeString += "Wait for Key Exchange Finished";
        break;
    case WAIT_FOR_CIPHER_FINISHED:
        nodeString += "Wait for Cipher Finished";
        break;
    case WAIT_FOR_DOWNLINK:
        nodeString += "Wait for Downlik";
        break;
    case REGISTERED:
        nodeString += "Registered. Wait for messagges";
        break;
    default:
        nodeString += status;
    }
    nodeString += "\n";
    
    return nodeString;
}

Node::Node () :
    keyValid (false),
    status (UNREGISTERED)
    //registered (false)
{
}

Node::Node (node_t nodeData) :
    keyValid(nodeData.keyValid), 
    lastMessageTime(nodeData.lastMessageTime), 
    lastMessageCounter(nodeData.lastMessageCounter), 
    nodeId(nodeData.nodeId), 
    status(nodeData.status)
    //registered(nodeData.registered)
{
    memcpy (key, nodeData.key, sizeof (uint16_t));
    memcpy (mac, nodeData.mac, 6);
}

NodeList::NodeList () {
    for (int i = 0; i < NUM_NODES; i++) {
        nodes[i].nodeId = i;
    }
}

Node *NodeList::getNodeFromID (uint16_t nodeId)
{
    return &(nodes[nodeId]);
}

Node *NodeList::getNodeFromMAC (const uint8_t * mac)
{
    uint16_t index = 0;

    while (index < NUM_NODES) {
        if (!memcmp (nodes[index].mac,mac,6)) {
            if (nodes[index].status != UNREGISTERED) { // TODO: check status??
                return &(nodes[index]);
            }
        }
        index++;
    }

    return NULL;
}

Node * NodeList::findEmptyNode ()
{
    uint16_t index = 0;

    while (index < NUM_NODES) {
        if (nodes[index].status != UNREGISTERED) { // TODO: check status??
            return &(nodes[index]);
        }
        index++;
    }

    return NULL;
}

uint16_t NodeList::countActiveNodes ()
{
    uint16_t counter = 0;

    for (int i = 0; i < NUM_NODES; i++) {
        if (nodes[i].status != UNREGISTERED) {
            counter++;
        }
    }
    return counter;
}

void Node::reset () {
    memset (mac, 0, 6);
    memset (key, 0, KEYLENGTH);
    keyValid = false;
    lastMessageCounter = 0;
    lastMessageTime = 0;
}

bool NodeList::unregisterNode (uint16_t nodeId)
{
    if (nodeId < NUM_NODES) {
        nodes[nodeId].reset ();

        if (nodes[nodeId].status != UNREGISTERED) {
            nodes[nodeId].status = UNREGISTERED;
            return true;
        } 
    }
    return false;
}

bool NodeList::unregisterNode (const uint8_t * mac)
{
    Node *node = getNodeFromMAC (mac);
    if (node) {
        node->reset ();
        node->status = UNREGISTERED;
        return true;
    } else {
        return false;
    }
}

bool NodeList::unregisterNode (Node *node)
{
    if (node)
    {
        node->reset ();

        if (nodes[node->nodeId].status != UNREGISTERED) {
            nodes[node->nodeId].status = UNREGISTERED;
            return true;
        } else {
            return false;
        }
    }
}

Node * NodeList::getNextActiveNode (uint16_t nodeId)
{
    for (int i = nodeId+1; i < NUM_NODES; i++) {
        if (nodes[i].status != UNREGISTERED) {
            return &(nodes[i]);
        }
    }
    return NULL;
}

Node * NodeList::getNextActiveNode (Node node)
{
    for (int i = node.nodeId + 1; i < NUM_NODES; i++) {
        if (nodes[i].status != UNREGISTERED) {
            return &(nodes[i]);
        }
    }
    return NULL;
}

Node * NodeList::getNewNode (const uint8_t * mac)
{
    Node *node = getNodeFromMAC (mac);
    if (node) {
        return node;
    } else {
        for (int i = 0; i < NUM_NODES; i++) {
            if (nodes[i].status == UNREGISTERED) {
                nodes[i].setMacAddress (mac);
                return &(nodes[i]);
            }
        }
    }
    return NULL;
}

String NodeList::toString () {
    String nodelistStr;

    for (int i = 0; i < NUM_NODES; i++) {
        if (nodes[i].status != UNREGISTERED) {
            nodelistStr += nodes[i].toString ();
        }
    }
    nodelistStr += "\n";
}
