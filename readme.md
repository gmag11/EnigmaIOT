# EnigmaIoT

<img src="https://github.com/gmag11/EnigmaIOT/raw/master/logo/logo%20text%20under.svg?sanitize=true" alt="EnigmaIoT Logo" width="50%"/>

[![ESP8266](https://github.com/gmag11/EnigmaIOT/workflows/ESP8266/badge.svg)](https://github.com/gmag11/EnigmaIOT/actions?query=workflow%3AESP8266)

[![ESP32](https://github.com/gmag11/EnigmaIOT/workflows/ESP32/badge.svg)](https://github.com/gmag11/EnigmaIOT/actions?query=workflow%3AESP32)

## Introduction

**EnigmaIoT** is an open source solution for wireless multi sensor systems. It has two main components, multiple **nodes** and one **gateway**.

A number of nodes with one or more sensors each one communicate in a **secure** way to a central gateway in a star network using EnigmaIoT protocol.

This protocol has been designed with security on mind. All node data is encrypted with a random key that changes periodically. Key is unique for each node and dynamically negotiated, so user do not have to enter any key. Indeed, all encryption and key agreement is transparent to user.

I designed this because I was searching for a way to have a relatively high number of nodes at home. I thought about using WiFi but it would overload my home router. So I looked for an alternative. I evaluated  LoRa or cheap nRF24 modules but I wanted the simplest solution in terms of hardware.

ESP8266 and ESP32 microcontrollers implement a protocol known as ESP-NOW. It is a point to point protocol, based on vendor specific [WiFi management action frames](https://mrncciew.com/2014/09/29/cwap-802-11-mgmt-frame-types/), that works in a connectionless fashion and every packet is a few milliseconds long. Because of this, it eases to have a battery powered node so that it enables designing totally wireless sensors.

But use of encryption on ESP-NOW limits the number of nodes to only 6 nodes. So I thought that I could implement encryption on payload but I found many problems I should solve to grade this as "secure enough".

Find library documentation on https://gmag11.github.io/EnigmaIOT

## Quick start

It you are courious to quickly test how does EnigmaIOT performs you can follow this [howto guide](docs/howto.md). This will guide you on how to:

- Setup your gateway
- Build simplest EnigmaIOT node
- Configure your first node
- Develop custom nodes with advanced features

## Project requirements

During this project conception I decided that it should fulfil this list of requirements.

- Use the simplest hardware, based on ESP8266 and/or ESP32.
- Secure by design. Make use of a secure channel for data transmission.
- Automatic dynamic key agreement.
- Do not require connection to the Internet.
- Do not overload my home WiFi infrastructure. Only Gateway will be connected to LAN.
- Able to use deep sleep to run on batteries.
- Enough wireless range for a house.
- Support for a high number of nodes.

## Features

- [x] Encrypted communication using [**ChaCha20/Poly1305**](https://tools.ietf.org/html/rfc7539)
- [x] Dynamic key, shared between one node and gateway. Keys are independent for each node
- [x] Shared keys are expired after a certain (configurable) time.
- [x] Number of nodes is only limited by memory on gateway (60 bytes per node)
- [x] Key is never on air so it is cannot be captured
- [x] Key expiration and renewal is managed transparently
- [x] Avoid repeatability attack having a new random initialization vector on every message. This is mandatory for ChaCha20/Poly1305 in order to keep cipher secure
- [x] Automatic and transparent node attachment
- [x] Avoid rogue node, rogue gateway and man-in-the-middle attack

Notice that network key used to implement this feature is stored on flash. ESP8266 do not allow flash encryption so network key may be recovered reading flash.

- [x] Pluggable physical layer communication. Right now only ESP-NOW protocol is developed but you can easily add more communication alternatives

- [x] When using ESP-NOW only ESP8266 or ESP32 is needed. No more electronics apart from sensor

- [x] Data message counter to detect lost or repeated messages

- [x] Designed as two libraries (one for gateway, one for node) for easier use

- [x] Crypto algorithm could be changed with low effort

- [x] Node and Gateway do store shared keys only on RAM. They are lost on power cycle. This protects system against flash reading attack. All nodes attach automatically with a new shared key after gateway is switched on

- [x] Downlink available. If deep sleep is used on sensor nodes, it is queued and sent just after node send a data message

- [x] Optional sleep mode management. In this case key and context has to be stored temporally. Normally RTC memory is the recommended place, and it is the one currently implemented. 
**Note**: There is the alternative to store context on flash memory so that node can be completely switched off between massages without requiring a new registering. Notice that on every received or sent message node updates this context  so consider that a high number of writes in flash may degrade it in the medium term. If messages counters are disabled in configuration the number of writes is decreased drastically but this reduces security level as it makes possible to repeat messages.

- [x] Initial configuration over WiFi portal on each device

- [x] Node configuration while in service using control downlink commands

- [ ] OTA over WiFi. Question: Is it really useful? Place an issue explaining an use case.

- [x] OTA over MQTT/ESP-NOW. Check [OTA script guide](docs/node-ota-update.md).

- [x] Node identification by using a flashing LED. This is useful when you have a bunch of nodes together :D

- [x] Broadcast messages that go to all nodes. This is implemented by sending messages to broadcast address (ff:ff:ff:ff:ff:ff in esp-now). Only nodes that are always listening are able to receive these messages, they are not queued. In order to send a broadcast message using EnigmaIOTGatewayMQTT you may use `<network name>/broadcast/...` as topi beginning. Any control or data message will arrive all nodes that have broadcast enabled. Control messages are processed normally except OTA and SET NAME, which are ignored. Data messages are passed to user code for processing.

  A shared encryption key is used to encrypt broadcast messages. It is generated automatically by Gateway on every restart. 

  If a node registers with broadcast flag active, gateway sends broadcast key using this message just after successful registration. Broadcast key is automatically generated on gateway on boot, so it will be different after every restart. Nodes will be synchronized as soon they register again.

  A node may not send broadcast messages, only gateway can.

- [x] Both gateway or nodes may run on ESP32 or ESP8266

- [x] Simple REST API to get information and send commands to gateway and nodes. Check [api.md](docs/api.md)

- [x] Node library includes methods to configure [Home Assistant](https://www.home-assistant.io) automatic integration using [MQTT discovery](https://www.home-assistant.io/docs/mqtt/discovery/)

## Technical background

If you want to know the internals about EnigmaIOT check [Technical Background Guide](docs/technical-background.md).

