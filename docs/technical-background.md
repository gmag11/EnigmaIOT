## Design

### System Design

System functions are divided in three layers: application, link and physical layer.

![Software Layers](../img/system_layers.png)

- **Application layer** is not controlled by EnigmaIoT protocol but main program. User may choose whatever data format or final destination of payload. A good option is to use CayenneLPP format but any other format or even raw data may be used. The only limit is the maximum packet length that, for ESP-NOW is around 200 bytes.

- **Link layer** is the one that add privacy and security. It manages connection between nodes and gateway in a transparent way. It does key agreement and node registration and checks the correctness of data messages. In case of any error it automatically start a new registration process. On this layer, data packets are encrypted using calculated symmetric key.

- **Physical layer** currently uses connectionless ESP-NOW. But a hardware abstraction layer has been designed so it is possible to develop interfaces for any other layer 1 technology like LoRa or nRF24F01 radios.

### EnigmaIoT protocol

The named **EnigmaIoT protocol** is designed to use encrypted communication without the need to hardcode the key. It uses [Elliptic Curves Diffie Hellman](https://en.wikipedia.org/wiki/Elliptic-curve_Diffie–Hellman) algorithm to calculate a shared key.

The process starts with node announcing itself with a Client Hello message. It tells the gateway its intention to establish a new shared key. It sends public key to be used on gateway to calculate the shared key.

Gateway answers with Server Hello message that includes its public key for shared key calculation on node.

Once shared key is calculated, node send an encrypted message as Key Exchange Finished message. Poly1305 encryption tag is used to check message integrity.

If gateway validates tag correctly it answers with a Cipher Finished message.

This process is protected with a 32 byte shared **network key**, used for **authentication**. As in the first two messages shared key is not known yet, Client Hello and Server Hello messages are encrypted whit this network key. If network key is not the same on gateway and node this will lead to decryption errors and messages will be ignored.

In case of any error on node key negotiation gateway sends an Invalidate Key to reset to original status and forgets any calculated shared key for this node.

When key is marked as valid node may start sending sensor data.

Optionally, gateway can send data to node. As node may be sleeping between communications, downlink messages has to be sent just after uplink data. So, one downlink message is queued until node communicates. Node waits some milliseconds before sleep for downlink data.

If a new downlink message arrives, old scheduled data for that node, if any, is overwritten.

In case of nodes that do not sleep (like a mains powered relay), gateway can send downlink data in any moment. Sleepy node is signaled during node registration on a bit in Key Exchange Finished message. It is set to 1 to signal that node will sleep just after sending data.

Key is forced to change every period. Gateway decides the moment to invalidate each node key. If so, it sends an invalidate key as downlink, after next data message communication. This key validity period is configurable on [EnigmaIoTconfig.h](https://github.com/gmag11/EnigmaIOT/blob/master/src/lib/EnigmaIoTconfig.h) file.

After that node may start new key agreement sending a new Client Hello message.

All nodes and gateway are identified by its MAC address. No name is assigned so no configuration is needed on node. Function assignment has to be done at a higher level.

## State diagram for nodes and Gateway

<img src="../img/StateDiagram-Sensor.svg?sanitize=true" alt="Sensor State Diagram" width="800"/>

<img src="../img/StateDiagram-Gateway.svg?sanitize=true" alt="Gateway State Diagram" width="800"/>

## Message format specification

### Client Hello message

![Client Hello message format](../img/ClientHello.png)

Client hello is sent by node to start registration procedure. It includes the public key to be used on Elliptic Curve Diffie Hellman (EDCH) algorithm to calculate the key. Initialization vector (IV) is used for encryption. There is a random 4 byte field reserved for future use.

This message is sent encrypted with network key.

### Server Hello message

![Server Hello message format](../img/ServerHello.png)

After receiving and checking Client Hello message, gateway responds with a Server Hello message. It carries gateway's public key to let node calculate key using ECDH. There is a random 4 byte field reserved for future use. Gateway assigns node a NodeID. It is signaled as a 2 byte field.

Server Hello message is sent encrypted with network key.

### Node Data message

![Node payload message format](../img/SensorData.png)

Node data is always encrypted using shared key and IV. Apart from payload this message includes node ID and a counter used by gateway to check lost or repeated messages from that node.

Total message length (without tag) is included on a 2 byte field.

### Unencrypted Node Data message

![Node unencrypted payload message format](../img/UnencryptedSensorData.png)

In case that extreme performance is needed there is the possibility to send unencrypted data so processor spends a few milliseconds less. It is not recommended to do so unless you want to investigate crypto software performance. Use at your own data risk :D

This message also includes node ID and a counter used by gateway to check lost or repeated messages from that node.

### Node control message (downlink)

![Node control message format](../img/SensorCommand-Downlink.png)

Gateway can send commands to an individual node in a similar way as sensor data is sent by nodes. For nodes that can be slept between consecutive data messages this commands are queued and sent just after a data message is received.

Only last message is queued. In case Gateway tries to send a new message, old one gets deleted and overridden by the new one.

Possible values of first byte means:

02:  SET command (unicast)

82: SET command (broadcast)

12: GET command (unicast)

92: GET command (broadcast)

### Control message

##### Downlink Control Message

![DL Control Command message format](../img/ControlComand-Downlink.png)

Broadcast messages of this type start with `0x84`.

##### Uplink Control Message

![UL Control Command message format](../img/ControlComand-Uplink.png)

Gateway  and node can exchange internal control commands. These are used to set internal protocol parameters like sleep time. This type of messages are processed like normal downlink messages, but are not passed to higher level (user code) in Node.

Some control messages, like OTA update messages, require that they are processed immediately. Hence, it is required that node is not in deep sleep mode. This can be controlled, for instance, using another control command to set sleep time to 0.

### Clock synchronization

##### Clock sync request

![Clock sync request](../img/ClockSyncRequest.png)

##### Clock sync response

![Clock sync response](../img/ClockSyncResponse.png)

In non sleepy nodes, it may be useful to send a message from time to time to let Gateway know that node is still active and let Node to check that is is still registered in Gateway.

Clock syncronization may be a very good feature if you need to coordinate actions on different nodes.

EnigmaIOT combines these two features into one request and response. Nodes may send clock sync request every some time to ping gateway and get common clock updated. Clock synchronization uses a mechanism similar to the one used by [SNTP protocol](https://en.wikipedia.org/wiki/Network_Time_Protocol#:~:text=Simple%20Network%20Time%20Protocol%20).

~~Notice that this is not world time sync but a numeric clock.~~

Since version 0.9.2, if Gateway has its internal time synchronized using NTP it sends non sleepy nodes **current real date and time** in millisecond Unix format .

This feature may be disabled if needed.

### Address to node name translation

##### Set node name

![Set Node Name](../img/SetNodeName.png)

##### Set node name result

![Set Node Name result](../img/SetNodeNameResult.png)

In order to make node messages more readable for humans, this implements a way to let Gateway to translate EnigmaIOT addresses to custom names (for instance, "RoomBlindControl"). This eases node replacement in case of failure.

Node names can be up to 32 characters long and should avoid characters different of letters and numbers. **Characters #,+ and / are specially forbidden**.

Node name is configured by user during first configuration in WiFi Web portal.

### Invalidate Key message

![Invalidate Key message format](../img/InvalidateKey.png)

After every data message from nodes, gateway evaluates key integrity and validity. In case of any error decoding the packet gateways ignores data and reply with this message indicating the reason that caused error. Node must start a new registration procedure in order to send data again. After this new registration node resends the last data message.

A gateway defines a key validity period after that a node key is marked as expired. In a message is received after that is processed normally but an Invalidate Key message indicating key expiration as reason. Node then starts a registration procedure but does not retry communication.

Invalidate Key message is always sent unencrypted.

## Protocol procedures

### Normal node registration and node data exchange

<img src="../img/NodeRegistration.svg?sanitize=true" alt="Normal node registration message sequence" width="400"/>

### Incomplete Registration

<img src="../img/RegistrationIncomplete.svg?sanitize=true" alt="Incomplete Registration message sequence" width="400"/>

### Node Not Registered

<img src="../img/NodeNotRegistered.svg?sanitize=true" alt="Node Not Registered message sequence" width="400"/>

### Key Expiration

<img src="../img/KeyExpiration.svg?sanitize=true" alt="KeyExpiration message sequence" width="400"/>

### Node Registration Collision

<img src="../img/NodeRegistrationCollision.svg?sanitize=true" alt="Node Registration Collision message sequence" width="600"/>

### Wrong Data Counter

<img src="../img/WrongCounter.svg?sanitize=true" alt="Wrong Counter message sequence" width="400"/>

## Hardware description

### Gateway

A gateway concentrates communication from all nodes, manages their registrations status, negotiate session key with them and outputs their messages to an output protocol.

[EnigmaIOT MQTT Gateway](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOTGatewayMQTT) is the implementation for a MQTT gateway.

Since version 0.7.0 Gateway is a ESP32 or ESP8266 board with 4 MB of flash memory or more. ESP8266 gateways cannot use MQTT TLS encryption due to memory limitations.

Use of ESP32 platform is recommended. ESP8266 EnigmaIOT gateway code is less tested.

Thanks to modular design, other output modules may be easily developed by implementing `GwOutput_generic.h`. Examples of this may be LoRaWAN output gateway, COAP gateway or any other network protocol that is needed. Even an offline SD data logger could be done.

I've included a [Gateway with dummy output module](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOTGatewayDummy) to show simple OutputGw module development.

In order to **configure** you need at least this data:

- **SSID**: WiFi network to connect to
- **Password**: WiFi pre shared key
- **Network Name**: Name of the EnigmaIOT network. If you have several gateways it is convenient to set them to different network names. Gateway sets up an AP with this name to help nodes to find its address and WiFi channel.
- **Network Key**: Encryption key to be used during node registration. It is used as Gateway AP pre shared key (not external WiFi). It must have from 8 to 32 characters long
- **Channel**: Initial WiFi channel to communicate nodes and Gateway. This is not important if gateway is connected to your home WiFi as nodes and gateway will use same channel as you have configured into your router.

User code may add additional custom parameters.

### Node

A node is either a ESP8266 or ESP32 board with a number of sensors. A node may sleep between sensor readings, status is stored so that it may send data without reconnection.

Any ESP8266 or ESP32 board with at least 1 MB of flash may be used.

There are several implementations in [examples](https://github.com/gmag11/EnigmaIOT/tree/master/examples):

[EnigmaIOT Node](https://github.com/gmag11/EnigmaIOT/tree/master/examples/enigmaiot_node): Basic node with deep sleep function. Sensor data is mocked up in example and sent using CayenneLPP encoding, you only need to replace it with your sensor reading code. Expected duration with 2 AA type batteries is more than one year, but a low power booster/regulator should be used in a custom board.

[Enigmaiot Node MsgPack](https://github.com/gmag11/EnigmaIOT/tree/master/examples/enigmaiot_node_msgpack): It has same functionality as the example above but uses JSON and MessagePack as Payload encoding.

[EnigmaIOT Node NonSleepy](https://github.com/gmag11/EnigmaIOT/tree/master/examples/enigmaiot_node_nonsleepy): Same functionality as previous examples but this does not sleep. This may be useful for sensors or actuators which are connected to mains, like light switches or smart plugs.

[EnigmaIOT LED Flasher](https://github.com/gmag11/EnigmaIOT/tree/master/examples/enigmaiot_led_flasher): On non sleepy nodes a common clock may be synchronized with gateway. This is an example of this. All nodes that include this firmware will flash their built in LED synchronously after successful registration. 

For **configuration**, node needs this data:

- **SSID**: corresponds to network name configured on Gateway
- **Password**: You must use network key used in Gateway
- **Node Name**: Human readable name for node. This must be unique in all nodes in same EnigmaIOT network. This may be changed afterwards using MQTT protocol
- **Sleep Time**: If a node uses deep sleep mode this configures the initial period to be slept. This can be modified using MQTT commands.

User code may add additional custom parameters.

## Data format

Although it is not mandatory at all, use of [CayenneLPP format](https://mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload) is recommended for sensor data compactness.

You may use [CayenneLPP library](https://github.com/ElectronicCats/CayenneLPP) for encoding on node and decoding on Gateway.

Example gateway code expands data message to JSON data, to be used easily as payload on a MQTT publish message to a broker. For JSON generation [ArduinoJSON](https://arduinojson.org) library is required.

In any case you can use your own format or even raw unencoded data. Take care of maximum message length that communications layer uses. For ESP-NOW, maximum payload length it is 217 bytes.

Since version 0.9 payload encoding is signaled on user data messages (both uplink and downlink) so new formats are possible. Currently  [CayenneLPP](https://mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload) and [MessagePack](https://msgpack.org) formats, in addition to RAW data, are possible. Check examples for usage instruction. MessagePack encoding and decoding are managed by ArduinoJSON library.

This change may produce incompatibilities with older versions so make sure you update your gateway and all your nodes to latest library version.

## ESP-NOW channel selection

Gateway has always its WiFi interface working as an AP. Its name corresponds to configured Network Name.

During first start, after connecting supply, node tries to search for a WiFi AP with that name. Whet it is found, node will use its MAC address and channel as destination for ESP-NOW messages. It also gets RSSI (signal level) and reports it to gateway.

This information is stored in flash so node will use it to communicate in all following messages.

In the case that gateway has changed its channel (for instance due to a reconfiguration) node will not be able to communicate again.

If several (2 by default) transmission errors are detected by node, it starts searching for gateway again. When found it keeps sending messages normally and new channel is updated in configuration persistently.

So, node will always follow the channel configuration that gateway is working in.

## Output data from gateway

### Uplink messages

A user may program their own output format modifying gateway example program. For my use case gateway outputs MQTT messages in this format:

```
<configurable prefix>/<node address | node name>/data <json data>
```

A prefix is configured on gateway to allow several sensor networks to coexist in the same subnet. After that address and data are sent.

After every received message, gateway detects if any packet has been lost before and reports it using MQTT message using this format:

```
<configurable prefix>/<node address | node name>/status {"per":<packet error rate>,"lostmessages":<Number of lost messages>,"totalmessages":<Total number of messages>,"packetshour":<Packet rate>}
```

### Downlink messages

EnigmaIoT allows sending messages from gateway to nodes. In my implementation I use MQTT to trigger downlink messages too.

To make it simpler, downlink messages use the same structure than uplink.

```
<network name>/<node address | node name>/<get|set>/data <command data>
```

Node address means destination node address. Configurable prefix is the same used for uplink communication.

Commands may be given in JSON format. In that case they are  sent to node in MessagePack format. That makes that mode gets the complete JSON object. This implies that no change is needed on Gateway to add new node types. Gateway is transparent to user data.

This is an example of MQTT message that triggers a downlink packet.

```
enigmaiot/12:34:56:78:90:12/set/data {"light1": 1, "light2": 0}
```

If node uses a name, MQTT message may use of it.

```
enigmaiot/kitcken_light/set/data {"light1": 1, "light2": 0}
```

After sending that command node will receive a 'set' command with data `{"light1": 1, "light2": 0}`.

Commands can be sent in any other format different that JSON, even binary. In that case they are sent without conversion to node using MessagePack encoding format to reduce transferred data bits.

### Control messages

Control messages are intended to set node specific settings, like sleep time, channel, trigger OTA update, etc. They are not passed to the main node sketch but gateway treat them as normal downlink messages.

Normally control commands trigger a response as an uplink message.

This is the list of currently implemented control commands:

- Get node protocol version
- Get/Set sleep duration time
- OTA Update
- Identify
- Node configuration reset
- Request measure RSSI
- Get/Set node name
- Restart node MCU

<table>
  <tr>
    <th colspan="2">Command</th>
    <th>Response</th>
  </tr>
  <tr>
    <td>Get version</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address  | node name&gt;/get/version</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/version {"version":"&lt;version&gt;"}</code></td>
  </tr>
  <tr>
    <td>Get sleep duration</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/get/sleeptime</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/sleeptime {"sleeptime":"&lt;sleep_time&gt;"}"</code></td>
  </tr>
  <tr>
    <td>Set sleep duration</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/set/sleeptime &lt;sleep_time&gt;</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/sleeptime {"sleeptime":"&lt;sleep_time&gt;"}</code></td>
  </tr>
  <tr>
    <td>OTA message</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/set/ota &lt;ota message&gt;</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/ota {"result":"&lt;ota_result_text&gt;,"status":"&lt;ota_result_code&gt;"}</code></td>
  </tr>
  <tr>
    <td>Identify node</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/set/identify</code></td>
    <td>None</td>
  </tr>
  <tr>
    <td>Reset node configuration</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/set/reset</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/reset {}</code></td>
  </tr>
  <tr>
    <td>Request measure RSSI</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/get/rssi</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/rssi {"rssi":&lt;RSSI&gt;,"channel":&lt;WiFi channel&gt;}</code></td>
  </tr>
  <tr>
    <td>Request node name</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/get/name</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/name {"address":&lt;node address&gt;,"name":&lt;Node name&gt;}</code></td>
  </tr>
  <tr>
    <td>Set node name</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/set/name &lt;Node name&gt;</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/result/name {"address":&lt;node address&gt;,"name":&lt;Node name&gt;}</code></td>
  </tr>
    <tr>
    <td>Restart Node MCU</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address | node name&gt;/set/restart</code></td>
    <td>None</td>
  </tr>
</table>




For instance, publishing `enigmaiot/12:34:56:78:90:12/get/version` will produce `enigmaiot/12:34:56:78:90:12/result/version 0.2.0`.

Messages are encoded to reduce the amount of bytes to be sent over internal protocol, so that the air time is as short as possible.

| Command                   | Msg type | Encoding                                                     |
| ------------------------- | -------- | ------------------------------------------------------------ |
| Get version               | `0x01`   | None                                                         |
| Version result            | `0x81`   | version as ASCII string                                      |
| Get sleep time            | `0x02`   | None                                                         |
| Set sleep time            | `0x03`   | Sleep time in seconds (Unsigned integer - 32 bit)            |
| Sleep time result         | `0x82`   | Sleep time in seconds (Unsigned integer - 32 bit)            |
| OTA Update                | `0xEF`   | OTA update specific format                                   |
| OTA Update result         | `0xFF`   | OTA result code (text and integer code)                      |
| Identify                  | `0x04`   | None. Function to identify a physical node by flashing its LED |
| Reset node configuration  | `0x05`   | None. This will set node to factory config                   |
| Reset config confirmation | `0x85`   | None                                                         |
| Request measure RSSI      | `0x06`   | None                                                         |
| Report measure RSSI       | `0x86`   | RSSI (signed integer - 8 bit), WiFi channel (unsigned integer - 8 bit) |
| Get node name             | `0x07`   | None                                                         |
| Report node name          | `0x87`   | Node name as string                                          |
| Set node name             | `0x08`   | Node name as string                                          |
| Restart node MCU          | `0x09`   | None                                                         |
| Send Broadcast Key        | `0x10`   | 32 byte key                                                  |

## OTA Update

MQTT Gateway example includes plain Arduino OTA mechanism. OTA is protected using network key selected during initial configuration.

On nodes, OTA updates are transferred using the same mechanism. Firmware is sent over MQTT using a [Python script](./EnigmaIoTUpdate/EnigmaIoTUpdate.py). Then gateway selects the appropriate node and send this binary data over ESP-NOW.

As ESP-NOW restricts **maximum payload to 250 bytes per message** firmware is splitted in chunks. Every chunk is **212 bytes** long, so that it fits together with message headers and is multiple of 4. This splitting work is done by `EnigmaIoTUpdate.py` script.

### Using EnigmaIoTUpdate.py

A requirement is to have installed [Python3](https://www.python.org/download/releases/3.0/) in the computer used to do the update.

In order to run the update, you need to install [`paho-mqtt`](https://pypi.org/project/paho-mqtt/) library. To do that you can follow instructions [here](https://pypi.org/project/paho-mqtt/#installation).

```
$python3 ./EnigmaIoTUpdate.py --help

Usage: EnigmaIoTUpdate.py [options]

Options:
  -h, --help            show this help message and exit
  -f FILENAME, --file=FILENAME
                        File to program into device
  -d ADDRESS, --daddress=ADDRESS
                        Node address or name
  -t BASETOPIC, --topic=BASETOPIC
                        Base topic for MQTT messages
  -u MQTTUSER, --user=MQTTUSER
                        MQTT server username
  -P MQTTPASS, --password=MQTTPASS
                        MQTT server user password
  -S MQTTSERVER, --server=MQTTSERVER
                        MQTT server address or name
  -p MQTTPORT, --port=MQTTPORT
                        MQTT server port
  -s, --secure          Use secure TLS in MQTT connection. Normally you should
                          use port 8883
  -D, --speed			Sets formware delivery speed [fast | medium | slow]. The fastest
                          the biggest chance to get troubles during update. Fast option
                          normally works but medium is more resilient
  --unsecure            Use secure plain TCP in MQTT connection. Normally you
                          should use port 1883
```

An example of this command could be like this:

```
python3 ./EnigmaIoTUpdate.py \
             -f EnigmaIOTsensor.bin \
             -d 11:22:33:44:55:66 \
             -t enigmaiot \
             -u "mymqttbrokeruser" \
             -P "mymqttbrokerpassword" \
             -S mysecure.mqtt.server \
             -p 8883 \
             -D medium \
             -s
```

Notice that using ESP-NOW, device address correspond to **MAC address** of your ESP8266 or ESP32 node.

It is very important to configure user and password on you MQTT broker. Besides, if it is going to be accessed from the Internet you should activate TLS encryption and a certificate.

## Home Assistant integration

JSON controller examples have integrated [Home Assistant autoconfiguration](https://www.home-assistant.io/docs/mqtt/discovery/). So, it is possible to design a node that autoregister automatically as soon it is connected to EnigmaIOT network.

You just need to add the specfic header files that correspond with your node profile. Currently these are implemented:

- [Sensors](https://www.home-assistant.io/integrations/sensor.mqtt/)
- [Binary sensors](https://www.home-assistant.io/integrations/binary_sensor.mqtt/)
- [Covers](https://www.home-assistant.io/integrations/cover.mqtt/)
- [Switches](https://www.home-assistant.io/integrations/switch.mqtt/)
- [Device Triggers](https://www.home-assistant.io/integrations/device_trigger.mqtt/)

Additionaly you need to add specific configuration in separate methods like this from `SmartSwitchController.cpp`

```c++
void CONTROLLER_CLASS_NAME::buildHASwitchDiscovery () {
    // Select corresponding HAEntiny type
    HASwitch* haEntity = new HASwitch ();

    uint8_t* msgPackBuffer;

    if (!haEntity) {
        DEBUG_WARN ("JSON object instance does not exist");
        return;
    }

    // *******************************
    // Add your characteristics here
    // There is no need to futher modify this function

    haEntity->setNameSufix ("switch");
    haEntity->setStateOn (1);
    haEntity->setStateOff (0);
    haEntity->setValueField ("rly");
    haEntity->setPayloadOff ("{\"cmd\":\"rly\",\"rly\":0}");
    haEntity->setPayloadOn ("{\"cmd\":\"rly\",\"rly\":1}");
    // *******************************

    size_t bufferLen = haEntity->measureMessage ();

    msgPackBuffer = (uint8_t*)malloc (bufferLen);

    size_t len = haEntity->getAnounceMessage (bufferLen, msgPackBuffer);

    DEBUG_INFO ("Resulting MSG pack length: %d", len);

    if (!sendHADiscovery (msgPackBuffer, len)) {
        DEBUG_WARN ("Error sending HA discovery message");
    }

    if (haEntity) {
        delete (haEntity);
    }

    if (msgPackBuffer) {
        free (msgPackBuffer);
    }
}

```

Finally you need to register every auto discovery methods in `connectInform` method.

```c++
void CONTROLLER_CLASS_NAME::connectInform () {

#if SUPPORT_HA_DISCOVERY    
    // Register every HAEntity discovery function here. As many as you need
    addHACall (std::bind (&CONTROLLER_CLASS_NAME::buildHASwitchDiscovery, this));
    addHACall (std::bind (&CONTROLLER_CLASS_NAME::buildHATriggerDiscovery, this));
    addHACall (std::bind (&CONTROLLER_CLASS_NAME::buildHALinkDiscovery, this));
#endif

    EnigmaIOTjsonController::connectInform ();
}
```

## External libraries

- ESPAsyncTCP -- https://github.com/me-no-dev/ESPAsyncTCP **(Required only for ESP8266)**
- AsyncTCP -- https://github.com/me-no-dev/AsyncTCP **(Required only for ESP32)**
- ESPAsyncWebServer -- https://github.com/me-no-dev/ESPAsyncWebServer
- ESPAsyncWiFiManager -- https://github.com/alanswx/ESPAsyncWiFiManager version > 0.22
- Arduino Crypto Library -- https://github.com/gmag11/CryptoArduino forked and formatted from https://github.com/rweather
- PubSubClient -- https://github.com/knolleary/pubsubclient
- CayenneLPP -- https://github.com/sabas1080/CayenneLPP version > 1.0.2
- ArduinoJSON 6 -- https://github.com/bblanchon/ArduinoJson version > 6.0.0

