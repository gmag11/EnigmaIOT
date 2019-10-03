# EnigmaIoT

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/logo/logo%20text%20under.svg?sanitize=true" alt="EnigmaIoT Logo" width="50%"/>

## Introduction

**EnigmaIoT** is an open source solution for wireless multi sensor systems. It has two main components, multiple **nodes** and one **gateway**.

A number of nodes with one or more sensors each one communicate in a **secure** way to a central gateway in a star network using EnigmaIoT protocol.

This protocol has been designed with security on mind. All sensor data is encrypted with a random key that changes periodically. Key is unique for each node and dynamically negotiated, so user do not have to enter any key. Indeed, all encryption and key agreement is transparent to user.

I designed this because I was searching for a way to have a relatively high number of nodes at home. I thought about using WiFi but it would overload my home router. So I looked for an alternative. I evaluated  LoRa or cheap nRF24 modules but I wanted the simplest solution in terms of hardware.

ESP8266 microcontroller implements a protocol known as ESP-NOW. It is a point to point protocol, based on vendor specific [WiFi management action frames](https://mrncciew.com/2014/09/29/cwap-802-11-mgmt-frame-types/), that works in a connectionless way and every packet is a few milliseconds long. Because of this it eases to have a battery powered node so that it enables designing totally wireless sensors.

But use of encryption on ESP-NOW limits the number of nodes to only 6 nodes. So I thought that I could implement encryption on payload but I found many problems I should solve to grade this as "secure enough".

Find library documentation on https://gmag11.github.io/EnigmaIOT

## Project requirements

During this project conception I decided that it should fulfil this list of requirements.

- Use the simplest hardware, based on ESP8266.
- Secure by design. Make use of a secure channel for data transmission.
- Automatic dynamic key agreement.
- Do not require connection to the Internet.
- Do not overload my home WiFi infrastructure. Only Gateway may be connected to LAN.
- Able to use deep sleep to run on batteries.
- Enough wireless range for a house.
- Support for a high number of nodes.

## Features

- [x] Encrypted communication using [**ChaCha20/Poly1305**](https://tools.ietf.org/html/rfc7539)
- [x] Dynamic key, shared between one node and gateway. Keys are independent for each node
- [x] Shared keys are expired after a certain (configurable) time
- [x] Number of nodes is only limited by memory on gateway (60 bytes per node)
- [x] Key is never on air so it is cannot be captured
- [x] Key expiration and renewal is managed transparently
- [x] Avoid repeatability attack having a new random initialization vector on every message. This is mandatory for ChaCha20/Poly1305 in order to keep cipher secure
- [x] Automatic and transparent node attachment
- [x] Avoid rogue node, rogue gateway and man-in-the-middle attack

Notice that network key used to implement this feature is stored on flash. ESP8266 do not allow flash encryption so network key may be recovered reading flash. On the other side, ESP32 is able to encrypt flash memory, but EnigmaIoT is not still tested on it.

- [x] Pluggable physical layer communication. Right now only ESP-NOW protocol is developed but you can easily add more communication alternatives
- [x] When using ESP-NOW only esp8266 is needed. No more electronics apart from sensor
- [x] Data message counter to detect lost or repeated messages
- [x] Designed as two libraries (one for gateway, one for node) for easier use
- [x] Crypto algorithm could be changed with low effort
- [x] Node and Gateway do store shared keys only on RAM. They are lost on power cycle. This protects system against flash reading attack. All nodes attach automatically with a new shared key after gateway is switched on
- [x] Downlink available. If deep sleep is used on sensor nodes, it is queued and sent just after node send a data message
- [x] Optional sleep mode management. In this case key has to be stored temporally. Normally RTC memory is the recommended place, and it is the one currently implemented
- [x] Initial configuration over WiFi portal on each device
- [x] Node configuration while in service using control downlink commands
- [ ] OTA over WiFi
- [x] OTA over MQTT/ESP-NOW
- [x] Sensor identification by using a flashing LED. This is useful when you have a bunch of nodes together :D
- [ ] Broadcast messages that go to all nodes. Under study
- [ ] ~~Node grouping to send messages to several nodes at once~~

## Design

### System Design

System functions are divided in three layers: application, link and physical layer.

![Software Layers](https://github.com/gmag11/EnigmaIOT/raw/master/img/system_layers.png)

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

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/StateDiagram-Sensor.svg?sanitize=true" alt="Sensor State Diagram" width="800"/>

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/StateDiagram-Gateway.svg?sanitize=true" alt="Gateway State Diagram" width="800"/>

## Message format specification

### Client Hello message

![Client Hello message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ClientHello.png)

Client hello is sent by node to start registration procedure. It includes the public key to be used on Elliptic Curve Diffie Hellman (EDCH) algorithm to calculate the key. Initialization vector (IV) is used for encryption. There is a random 4 byte field reserved for future use.

This message is sent encrypted with network key.

### Server Hello message

![Server Hello message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ServerHello.png)

After receiving and checking Client Hello message, gateway responds with a Server Hello message. It carries gateway's public key to let node calculate key using ECDH. There is a random 4 byte field reserved for future use. Gateway assigns node a NodeID. It is signaled as a 2 byte field.

Server Hello message is sent encrypted with network key.

### Sensor Data message

![Node payload message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/SensorData.png)

Sensor data is always encrypted using shared key and IV. Apart from payload this message includes node ID and a counter used by gateway to check lost or repeated messages from that node.

Total message length (without tag) is included on a 2 byte field.

### Node control message (downlink)

![Node control message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/SensorCommand-Downlink.png)

Gateway can send commands to an individual node in a similar way as sensor data is sent by nodes. For nodes that can be slept between consecutive data messages this commands are queued and sent just after a data message is received.

Only last message is queued. In case Gateway tries to send a new message, old one gets deleted and overridden by the new one.

### Control message

##### Downlink Control Message

![DL Control Command message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ControlComand-Downlink.png)

##### Uplink Control Message

![UL Control Command message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ControlComand-Uplink.png)

Gateway  and node can exchange internal control commands. These are used to set internal protocol parameters like sleep time. This type of messages are processed like normal downlink messages, but are not passed to higher level (user code) in Node.

Some control messages, like OTA update messages, require that they are processed immediately. Hence, it is required that node is not in deep sleep mode. This can be controlled, for instance, using another control command to set sleep time to 0.

### Invalidate Key message

![Invalidate Key message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/InvalidateKey.png)

After every data message from nodes, gateway evaluates key integrity and validity. In case of any error decoding the packet gateways ignores data and reply with this message indicating the reason that caused error. Node must start a new registration procedure in order to send data again. After this new registration node resends the last data message.

A gateway defines a key validity period after that a node key is marked as expired. In a message is received after that is processed normally but an Invalidate Key message indicating key expiration as reason. Node then starts a registration procedure but does not retry communication.

Invalidate Key message is always sent unencrypted.

## Protocol procedures

### Normal node registration and sensor data exchange

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/NodeRegistration.svg?sanitize=true" alt="Normal node registration message sequence" width="400"/>

### Incomplete Registration

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/RegistrationIncomplete.svg?sanitize=true" alt="Incomplete Registration message sequence" width="400"/>

### Node Not Registered

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/NodeNotRegistered.svg?sanitize=true" alt="Node Not Registered message sequence" width="400"/>

### Key Expiration

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/KeyExpiration.svg?sanitize=true" alt="KeyExpiration message sequence" width="400"/>

### Node Registration Collision

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/NodeRegistrationCollision.svg?sanitize=true" alt="Node Registration Collision message sequence" width="600"/>

### Wrong Data Counter

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/WrongCounter.svg?sanitize=true" alt="Wrong Counter message sequence" width="400"/>

## Hardware description

### Node

tbd.

### Gateway

tbd.

## Data format

Although it is not mandatory at all, use of [CayenneLPP format](https://mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload) is recommended for sensor data compactness.

You may use [CayenneLPP library](https://github.com/ElectronicCats/CayenneLPP) for encoding on node and decoding on Gateway.

Example gateway code expands data message to JSON data, to be used easily as payload on a MQTT publish message to a broker. For JSON generation [ArduinoJSON](https://arduinojson.org) library is required.

In any case you can use your own format or even raw unencoded data. Take care of maximum message length that communications layer uses. For ESP-NOW, maximum payload length it is 217 bytes.

## ESP-NOW channel selection

Gateway has always its WiFi interface working as an AP. Its name correspond to configured Network Name.

During first start, after connecting supply, node tries to search for a WiFi AP with that name. Whet it is found, node will use its MAC address and channel as destination for ESP-NOW messages. It also gets RSSI (signal level) and reports it to gateway.

This information is stored in flash so node will use it to communicate in all following messages.

In the case that gateway has changed its channel (for instance due to a reconfiguration) node will not be able to communicate again.

If several transmission errors are detected by node, it starts searching for gateway again. When found it keeps sending messages normally.

So, node will always follow the channel that gateway is working in.

## Output data from gateway

### Uplink messages

A user may program their own output format modifying gateway example program. For my use case gateway outputs MQTT messages in this format:
```
<configurable prefix>/<node address>/sensordata <json data>
```
A prefix is configured on gateway to allow several sensor networks to coexist in the same subnet. After that address and data are sent.

After every received message, gateway detects if any packet has been lost before and reports it using MQTT message using this format:
```
<configurable prefix>/<node address>/status {"per":<packet error rate>,"lostmessages":<Number of lost messages>,"totalmessages":<Total number of messages>,"packetshour":<Packet rate>}
```
If ESP-NOW is used for node communication, ESP8266 cannot use WiFi in a reliable way. That's why in this case gateway will be formed by two ESP8266 linked by serial port. First one will output MQTT data in form of ascii strings and second one will forward them to MQTT broker.

### Downlink messages

EnigmaIoT allows sending messages from gateway to nodes. In my implementation I use MQTT to trigger downlink messages too.

To make it simpler, downlink messages use the same structure than uplink.
```
<configurable prefix>/<node address>/<command> <data>
```
Node address means destination node address. Configurable prefix is the same used for uplink communication.

Commands are developed by user, but some are reserved for control commands. An uplink message could be like this

```
enigmaiot/12:34:56:78:90:12/light ON
```

### Control messages

Control messages are intended to set node specific settings, like sleep time, channel, trigger OTA update, etc. They are not passed to the main sketch but gateway treat them as normal downlink messages.

Normally control commands trigger a response as an uplink message.

This is the list of currently implemented control commands:

- Get node protocol version
- Get sleep duration time
- Set sleep duration time
- OTA Update
- Identify
- Node configuration reset

<table>
  <tr>
    <th colspan="2">Command</th>
    <th>Response</th>
  </tr>
  <tr>
    <td>Get version</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/get/version</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/result/version &lt;version&gt;</code></td>
  </tr>
  <tr>
    <td>Get sleep duration</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/get/sleeptime</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/result/sleeptime &lt;sleep_time&gt;</code></td>
  </tr>
  <tr>
    <td>Set sleep duration</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/set/sleeptime &lt;sleep_time&gt;</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/result/sleeptime &lt;sleep_time&gt;</code></td>
  </tr>
  <tr>
    <td>OTA message</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/set/ota &lt;ota message&gt;</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/result/ota &lt;ota_result_code&gt;</code></td>
  </tr>
  <tr>
    <td>Identify node</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/set/identify</code></td>
    <td>None</td>
  </tr>
  <tr>
    <td>Reset node configuration</td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/set/reset</code></td>
    <td><code>&lt;configurable prefix&gt;/&lt;node address&gt;/result/reset</code></td>
  </tr>
</table>

For instance, publishing `enigmaiot/12:34:56:78:90:12/get/version` will produce `enigmaiot/12:34:56:78:90:12/result/version 0.2.0`.

Messages are encoded to reduce the amount of bytes to be sent over internal protocol, so that the air time is as short as possible.

| Command | Msg type | Encoding |
| ------- | -------- | -------- |
| Get version | `0x01` | None |
| Version result | `0x81` | version as ASCII string |
| Get sleep time | `0x02` | None |
| Set sleep time | `0x03` | Sleep time in seconds (Unsigned integer - 32 bit) |
| Sleep time result | `0x82` | Sleep time in seconds (Unsigned integer - 23 bit) |
| OTA Update | `0xEF` | OTA update specific format |
| OTA Update result | `0xFF` | OTA result code |
| Identify | `0x04` | None. Function to identify a physical node by flashing its LED |
| Reset node configuration | `0x05` | None. This will set node to factory config |
| Reset config confirmation | `0x85` | None |

## OTA Update

OTA updates are transferred using the same mechanism. Firmware is sent over MQTT using a [Python script](./EnigmaIoTUpdate/EnigmaIoTUpdate.py). Then gateway selects the appropriate node and send this binary data over ESP-NOW.

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
                        Device address
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
  -D, --speed			Sets formware delivery speed [fast | medium | slow]. The fastest 						   the biggest chance to get troubles during update. Fast option 						   normally works but medium is more resilient
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

Notice that using ESP-NOW device address correspond to **MAC address** of your ESP8266.

It is very important to configure user and password on you MQTT broker. Besides, if it is going to be accessed from the Internet you should activate TLS encryption and a certificate.

## External libraries

- CRC32 -- https://github.com/bakercp/CRC32
- ESPAsyncWebServer -- https://github.com/me-no-dev/ESPAsyncWebServer
- ESPAsyncWiFiManager -- https://github.com/alanswx/ESPAsyncWiFiManager version > 0.22
  
- Arduino Crypto Library -- https://github.com/gmag11/CryptoArduino forked and formatted from https://github.com/rweather
- ~~ESP8266TrueRandom -- https://github.com/marvinroger/ESP8266TrueRandom~~
- PubSubClient -- https://github.com/knolleary/pubsubclient
- CayenneLPP -- https://github.com/sabas1080/CayenneLPP version > 1.0.2
- ArduinoJSON 6 -- https://github.com/bblanchon/ArduinoJson

