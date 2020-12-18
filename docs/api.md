## Server API

Since version 0.9.6 of EnigmaIOT, network connected Gateways may include a REST API to get information and manage nodes.

This enables future development of a web frontend for EnigmaIOT Gateways.

All requests parameters are sent as URL encoded.

All responses are given in JSON format

### Gateway information

| Entry point        | Parameters | Method | Response                                                     | Comments                                                     |
| ------------------ | ---------- | ------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `/api/gw/info`     |            | GET    | **version**: EnigmaIOT library version<br/>**network**: EnigmaIOT network name<br/>**addresses**: <br/>    **AP**: Gateway AP mac address<br/>    **STA**: Gateway STA mac address<br/>**channel**: WiFi channel used<br/>**ap**: AP name<br/>**bssid**: AP mac address<br/>**rssi**: AP RSSI (dBm)<br/>**txpower**: Gateway WiFi power (dBm)<br/>**dns**: DNS Address | Gets gateway network information                             |
| /api/gw/nodenumber |            | GET    | **nodeNumber**: Number of registered nodes                   | Gets current number of registered nodes                      |
| /api/gw/maxnodes   |            | GET    | **maxNodes**: Maximum number of nodes allowed                | Gets the maximum number of nodes that can be registered in gateway |



### Gateway commands

| Entry point    | Parameters | Method | Response                            | Comments                                        |
| -------------- | ---------- | ------ | ----------------------------------- | ----------------------------------------------- |
| api/gw/restart | confirm=1  | PUT    | **gw_restart**: <processed \| fail> | Restarts gateway software. Confirm must be 1    |
| api/gw/reset   | confirm=1  | PUT    | **gw_reset**: <processed \| fail>   | Resets gateway configuration. Confirm must be 1 |



### Node information

| Entry point    | Parameters | Method | Response                                                     | Comments                                                     |
| -------------- | ---------- | ------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| /api/gw/nodes  |            | GET    | **nodes**: `<list>`<br/>    **nodeId**: Node identifier assigned by gateway<br/>    **address**: Node mac address<br/>    **name**: Node name | Gets a list of registered nodes with nodeId, address and name |
| /api/node/node | nodeid     | GET    | **version**: EnigmaIOT library version<br/>**node_id**: NodeID<br/>address: Node mac address<br/>**Name**: Node name<br/>**keyValidSince**: Time since session key was last refreshed (seconds)<br/>**lastMessageTime**: Time since last message (seconds)<br/>**sleepy**: True \| False<br/>**broadcast**: True \| False<br/>**rssi**: Received gateway power from node<br/>**packetsHour**: Packet rate (pkt/h)<br/>**per**: Packet error rate (%) | Gets node information given its nodeID                       |
| /api/node/node | nodename   | GET    | **version**: EnigmaIOT library version<br/>**node_id**: NodeID<br/>address: Node mac address<br/>**Name**: Node name<br/>**keyValidSince**: Time since session key was last refreshed (seconds)<br/>**lastMessageTime**: Time since last message (seconds)<br/>**sleepy**: True \| False<br/>**broadcast**: True \| False<br/>**rssi**: Received gateway power from node<br/>**packetsHour**: Packet rate (pkt/h)<br/>**per**: Packet error rate (%) | Gets node information given its name                         |
| /api/node/node | nodeaddr   | GET    | **version**: EnigmaIOT library version<br/>**node_id**: NodeID<br/>address: Node mac address<br/>**Name**: Node name<br/>**keyValidSince**: Time since session key was last refreshed (seconds)<br/>**lastMessageTime**: Time since last message (seconds)<br/>**sleepy**: True \| False<br/>**broadcast**: True \| False<br/>**rssi**: Received gateway power from node<br/>**packetsHour**: Packet rate (pkt/h)<br/>**per**: Packet error rate (%) | Gets node information given its mac address                  |



### Node commands

| Entry point       | Parameters | Method | Response                       | Comments                               |
| ----------------- | ---------- | ------ | ------------------------------ | -------------------------------------- |
| /api/node/node    | nodeid     | DEL    | **result**: Error string       | Unregisters node given its nodeID      |
| /api/node/node    | nodename   | DEL    | **result**: Error string       | Unregisters node given its name        |
| /api/node/node    | nodeaddr   | DEL    | **result**: Error string       | Unregisters node given its mac address |
| /api/node/restart | nodename   | PUT    | **node_restart**: Error string | Triggers node restart                  |

