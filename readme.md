# EnigmaIoT

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/logo/logo%20text%20under.svg?sanitize=true" alt="EnigmaIoT Logo" width="50%"/>

## Introduction

**EnigmaIoT** is an open source solution for wireless multi sensor systems. It has two main components, multiple **nodes** and one **gateway**.

A number of nodes with one or more sensors each one communicate in a **secure** way to a central gateway in a star network using EnigmaIoT protocol.

This protocol has been designed with security on mind. All sensor data is encrypted with a random key that changes periodically. Key is unique for each node and dynamically calculated, so user do not have to enter any key. Indeed, all encryption and key agreement is transparent to user.

I designed this because I was searching for a way to have a relatively high number of nodes at home. I thought about using WiFi but it would overload my home router. So I looked for an alternative. I thought about LoRa or cheap 2.4GHz modules but I wanted the simplest solution in terms of hardware.

ESP8266 microcontroller implements a protocol known as ESP-NOW. It is a point to point protocol, based on special WiFi frames, that works in a connectionless way and every packet is short in time. Because of this it eases to have a battery powered node so that it enables designing totally wireless sensors.

But use of encryption on ESP-NOW limits the number of nodes to only 6 nodes. So I thought that I could implement encryption on payload but I found many problems I should solve to grade this as "secure enough".

Find library documentation on https://gmag11.github.io/EnigmaIOT

## Project requirements

During this project conception I decided that it should fulfil this list of requirements.

- Use the simplest hardware, based on ESP8266.
- Secure by design. Make use of a secure channel for data transmission.
- Automatic dynamic key agreement.
- Do not require connection to the Internet.
- Do not overload my home WiFi infrastructure.
- Able to use deep sleep to run on batteries.
- Enough wireless range for a house.
- Support for a high number of nodes.

## Features

- [x] Encrypted communication
- [x] Dynamic key, shared between one node and gateway. Keys are independent for each node
- [x] Number of nodes is only limited by memory on gateway (56 bytes per node)
- [x] Key is never on air so it is not interceptable
- [x] Key expiration and renewal is managed transparently
- [x] Avoid repeatability attack having a new random initialization vector on every message
- [x] Automatic and transparent node attachment
- [x] Avoid rogue node, rogue gateway and man-in-the-middle attack.

Notice that network key used to implement this feature is stored on flash. ESP8266 do not allow flash encryption so network key may be recovered reading flash. On the other side, ESP32 is able to encrypt flash memory, but EnigmaIoT is not still tested on it.

- [x] Pluggable physical layer communication. Right now only ESP-NOW protocol is developed but you can easily add more communication alternatives
- [x] When using ESP-NOW only esp8266 is needed. No more electronics apart from sensor
- [x] Optional data message counter to detect lost or repeated messages
- [x] Designed as two libraries (one for gateway, one for node) for easier use.
- [x] Selectable crypto algorithm
- [x] Node and Gateway do store shared keys only on RAM. They are lost on power cycle. This protects system against flash reading attack. All nodes attach automatically after gateway is switched on
- [x] Downlink available. If deep sleep is used on sensor nodes, it is queued and sent just after node send a data message
- [x] Optional sleep mode management. In this case key has to be stored temporally. Normally RTC memory is the recommended place, and it is the one currently implemented, but SPIFFS or EEPROM would be possible.
- [x] Initial configuration over WiFi portal on each device
- [x] Node configuration on service using control downlink commands
- [ ] OTA over WiFi
- [ ] OTA over MQTT/ESP-NOW (**under development**)

## Design

### System Design

System functions are divided in three layers: application, link and physical layer.

![Software Layers](https://github.com/gmag11/EnigmaIOT/raw/master/img/system_layers.png)

- **Application layer** is not controlled by EnigmaIoT protocol but main program. User may choose whatever data format or final destination of payload. A good option is to use CayenneLPP format but any other format or even raw data may be used. The only limit is the maximum packet length that, for ESP-NOW is around 200 bytes.

- **Link layer** is the one that add privacy and security. It manages connection between nodes and gateway in a transparent way. It does key agreement and node registration and checks the correctness of data messages. In case of any error it automatically start a new registration process. On this layer, data packets are encrypted using calculated symmetric key.

- **Physical layer** currently uses connectionless ESP-NOW. But a hardware abstraction layer has been designed so it is possible to develop interfaces for any other layer 1 technology like LoRa or nRF24F01 radios.

### EnigmaIoT protocol

The named **EnigmaIoT protocol** is designed to use encrypted communication without the need to hardcode the key. It uses [Diffie Hellman](https://security.stackexchange.com/a/196480) algorithm to calculate a shared key.

The process starts with node announcing itself with a Client Hello message. It tells the gateway its intention to establish a new shared key. It sends Diffie Hellman public part to be used on gateway to calculate the key.

Gateway answers with Server Hello message that includes its public data for key calculation on node.

Once key is calculated, node send an encrypted message as Key Exchange Finished message. A 32 bit CRC is calculated and it is used for decryption test.

If gateway validates CRC correctly it answers with a Cipher Finished message. It carries a CRC too.

This process is protected with a 32 byte shared **network key**, used for **authentication**. If network key is not the same on gateway and node this will lead to CRC errors.

In case of any error in this process gateway sends an Invalidate Key to reset to original status and forgets key.

When key is marked as valid node may start sending sensor data.

Optionally gateway can send data to node. As node may be sleeping between communications, downlink messages has to be sent just after uplink data. So, one downlink message is queued until node communicates. Node waits some milliseconds before sleep for downlink data.

In case of nodes that do not sleep (like actuators) gateway can send downlink data in any moment. Sleepy node is signaled during node registration on a bit on Key Exchange Finished message. It is set to 1 to signal that node will sleep just after sending data.

Key is forced to change every period. Gateway decides the moment to invalidate each node key. If so, it sends an invalidate key as downlink, after next data message communication.

After that node may start new key agreement sending a new Client Hello message.

All nodes and gateway are identified by its MAC address. No name is assigned so no configuration is needed on node. Function assignment has to be done in a  higher level.

## State diagram for nodes and Gateway

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/StateDiagram-Sensor.svg?sanitize=true" alt="Sensor State Diagram" width="800"/>

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/img/StateDiagram-Gateway.svg?sanitize=true" alt="Gateway State Diagram" width="800"/>

## Message format specification

### Client Hello message

![Client Hello message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ClientHello.png)

Client hello is sent by node to start registration procedure. It includes the public key to be used on Diffie Hellman algorithm to calculate the key. A random filled 16 byte field is reserved for future use.

This message is sent unencrypted.

### Server Hello message

![Server Hello message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ServerHello.png)

After receiving and checking Client Hello message, gateway responds with a Server Hello message. It carries gateway's public key to let node calculate key using DH. There is a random 16 byte field reserved for future use.

Server Hello message is sent unencrypted.

### Key Exchange Finished message

![Key Exchange Finished message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/KeyExchangeFinished.png)

After node has calculated shared key it generates a Key Exchange Finished message filled with random data and a CRC. These fields are encrypted using shared key and an initialization value (IV) that is sent unencrypted.

It is used by gateway to check that calculated shared key is correct.

### Cypher Finished message

![Cypher Finished message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/CypherFinished.png)

After gateway has decoded correctly Key Exchange Finished message, it build a Cypher Finished message to let node check that key is correct. Gateway assigns node a NodeID. It is signaled as a 2 byte field.

### Sensor Data message

![Sensor Data message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/SensorData.png)

Sensor data is always encrypted using shared key and IV. Apart from payload this message includes node ID and a counter used by gateway to check lost or repeated messages from that node.

Total message length is included on a 2 byte field.

### Sensor Command message (downlink)

![Sensor Command message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/SensorCommand-Downlink.png)

Gateway can send commands to an individual node in a similar way as sensor data is sent by nodes. For nodes that can be slept between consecutive data messages, this commands are queued and sent just after a data message is received.

Only last message is queued. In case Gateway tries to send a new message, old one gets deleted and overridden by the new one.

### Control Command message (downlink)

![DL Control Command message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ControlComand-Downlink.png)
![UL Control Command message format](https://github.com/gmag11/EnigmaIOT/raw/master/img/ControlComand-Uplink.png)

Gateway  and node can exchange internal control commands. These are used to set internal protocol parameters. This type of messges are processed like normal downlink messages, but are not passed to higher level in Node.

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

## Data format

Although it is not mandatory at all, use of [CayenneLPP format](https://mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload) is recommended for sensor data compactness.

You may use [CayenneLPP encoder library](https://github.com/sabas1080/CayenneLPP) on sensor node and [CayenneLPP decoder library](https://github.com/gmag11/CayenneLPPdec) for decoding on Gateway.

Example gateway code expands data message to JSON data, to be used easily as payload on a MQTT publish message to a broker. For JSON generation ArduinoJSON library is required.

In any case you can use your own format or even raw unencoded data. Take care of maximum message length that communications layer uses. For ESP-NOW it is about 200 bytes.

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
- ~~OTA Update~~

| Command | Response |
| ------- | -------- |
| `<configurable prefix>/<node address>/get/version` | `<configurable prefix>/<node address>/result/version <version>` |
| `<configurable prefix>/<node address>/get/sleeptime` | `<configurable prefix>/<node address>/result/sleeptime <sleep_time>` |
| `<configurable prefix>/<node address>/set/sleeptime <sleep_time>` | `<configurable prefix>/<node address>/result/sleeptime <sleep_time>` |
| `<configurable prefix>/<node address>/get/ota` | `<configurable prefix>/<node address>/result/ota <ota_result_code>` |

For instance, publishing `enigmaiot/12:34:56:78:90:12/get/version` will produce `enigmaiot/12:34:56:78:90:12/result/version 0.2.0`.

Messages are encoded to reduce the amount of bytes to be sent over internal protocol, so that the air time is as short as possible.

| Command | Msg type | Encoding |
| ------- | -------- | -------- |
| Get version | `0x01` | |
| Version result | `0x81` | version in ASCII |
| Get sleep time | `0x02` | |
| Set sleep time | `0x02` | Sleep time in seconds (Unsigned integer - 4 bytes) |
| Sleep time result | `0x82` | Sleep time in seconds (Unsigned integer - 4 bytes) |
| OTA Update | `0xEF` | OTA update specific format |
| OTA Update result | `0xFF` | tbd. |

## OTA Update

tbd.

## External libraries

- CRC32 -- https://github.com/bakercp/CRC32
- ESPAsyncWebServer -- https://github.com/me-no-dev/ESPAsyncWebServer
- ESPAsyncWiFiManager -- https://github.com/alanswx/ESPAsyncWiFiManager
- Arduino Crypto Library -- https://github.com/rweather/arduinolibs
  - This one needs a modification in order to run successfully on ESP8266 Arduino core > 2.5.x. **You have to change line 30** on `Crypto/BigNumberUtil.h` from `#if defined (__AVR__) || defined(ESP8266)` to `#if defined (__AVR__)`. **Without this, code will crash**.
  - There are some objects that cause conflicts if you are using windows due to capitalization. Files `SHA1.cpp` and `SHA1.h` **have to be deleted** from `CryptoLegacy/src`
- PubSubClient -- https://github.com/knolleary/pubsubclient
- CayenneLPP -- https://github.com/sabas1080/CayenneLPP
- CayenneLPPDec -- https://github.com/gmag11/CayenneLPPdec
- ArduinoJSON 6 -- https://github.com/bblanchon/ArduinoJson