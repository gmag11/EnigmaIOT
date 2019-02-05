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

bool NodeList::unregisterNode (uint16_t nodeId)
{
    if (nodeId < NUM_NODES) {
        memset (nodes[nodeId].mac, 0, 6);
        memset (nodes[nodeId].key, 0, KEYLENGTH);
        nodes[nodeId].keyValid = false;

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
        memset (node->mac, 0, 6);
        memset (node->key, 0, KEYLENGTH);
        node->keyValid = false;
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
        memset (node->mac, 0, 6);
        memset (node->key, 0, KEYLENGTH);
        node->keyValid = false;

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
