// 
// 
// 

#include "NodeList.h"

void Node::setEncryptionKey (uint8_t* key) {
    if (key) {
        memcpy (this->key, key, KEYLENGTH);
    }
}

node_t Node::getNodeData () {
    node_t thisNode;

    memcpy (thisNode.key, key, KEYLENGTH);
    thisNode.keyValid = keyValid;
    thisNode.lastMessage = lastMessage;
    memcpy (thisNode.mac, mac, 6);
    thisNode.nodeId = nodeId;
}

Node::Node (node_t nodeData) : keyValid(nodeData.keyValid), lastMessage(nodeData.lastMessage), nodeId(nodeData.nodeId) {
    memcpy (key, nodeData.key, sizeof (uint16_t));
    memcpy (mac, nodeData.mac, 6);
}

Node *NodeList::getNode (uint16_t nodeId)
{
    return &(nodes[nodeId]);
}

Node *NodeList::getNode (uint8_t * mac)
{
    bool found = false;
    uint16_t index = 0;

    while (!found && index <= NUM_NODES) {
        if (!memcmp (nodes[index].mac,mac,6)) {
            return &(nodes[index]);
        }
        index++;
    }

    return NULL;
}
