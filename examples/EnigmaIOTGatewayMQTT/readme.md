# EnigmaIOT Gateway MQTT

This example implements a complete EnigmaIOT gateway that sends data and allow interacting using MQTT protocol. It uses TLS connection to broker, so you can use to connect a broker in a remote place on Internet.

You may use in your projects as it is, without any modification.

It uses [`GwOutput_generic.h`](https://github.com/gmag11/EnigmaIOT/blob/master/src/GwOutput_generic.h) to implement MQTT communication as a module.

[`dstrootca.h`](https://github.com/gmag11/EnigmaIOT/blob/master/examples/EnigmaIOTGatewayMQTT/dstrootca.h) includes **DST Root CA X3** certificate, valid until 2021-09-30 14:01­:15 UTC. It is the root CA for Let's Encrypt certificates, so if you use a different one or expiry date has passed make sure that you update the file with the right certificate.

If you have a MQTT broker without TLS activated you have to disable [`#include "dstrootca.h"`](https://github.com/gmag11/EnigmaIOT/blob/master/examples/EnigmaIOTGatewayMQTT/GwOutput_mqtt.h#L21) on [`GwOutput_mqtt.h`](https://github.com/gmag11/EnigmaIOT/blob/master/examples/EnigmaIOTGatewayMQTT/GwOutput_mqtt.h)  

## Data Format

Gateway does not interpret neither store any node data. It is completely transparent for payload.

### Output data

Data from EnigmaIOT nodes is sent through MQTT using this topic format

```
<Network_Name>/<Node_address or Node_name>/data
```

Payload will have JSON format if node data is encoded as CayenneLPP or MsgPack. In any other case payload will be sent without any transformation.

### Input Data

Nodes may accept user commands to ask for information or adjust settings. There are only two topics that may be used

`<Network_Name>/<Node_address or Node_name>/get/data` to ask node for information

`<Network_Name>/<Node_address or Node_name>/set/data` to configure settings

If payload is given as JSON, EnigmaIOT Gateway MQTT encodes it as MsgPack. So node will get the same JSON object.

If payload is not a JSON object it will be sent transparently.

## Configuration

Gateway is configured during first boot using a web portal. If not configured or it is not able to connect to WiFi router, Gateway starts an AP. Use your smartphone to connect to EnigmaIOTGateway network and access to http://192.168.4.1.

You need to provide your WiFi name and password, a name and password for your EnigmaIOT network. It asks for a WiFi channel. You do not need to configure this as it will use your WiFi network channel.

In addition you need to enter MQTT broker address and port and username and password.

