# EnigmaIOT Led Controller

This shows the simplest implementation of [EnigmaIOTjsonController.h](https://github.com/gmag11/EnigmaIOT/blob/dev/src/EnigmaIOTjsonController.h) that accepts commands.

Led can be turned on by sending the right command.

Using [EnigmaIOT Gateway MQTT](https://github.com/gmag11/EnigmaIOT/tree/dev/examples/EnigmaIOTGatewayMQTT), this node get commands if MQTT messages use this format.

```
<Network_name>/<node_name or node_address>/set/data  {"cmd":"led","led":< 1 or 0 >}
```

- 1 means led on
- 0 means led off

You may interrogate node for led status in any moment by sending this message

```
<Network_name>/<node_name or node_address>/get/data  {"cmd":"led"}
```

Node will answer with a message using this format.

```
<Network_name>/<node_name or node_address>/set/data  {"cmd":"led","led":< 1 or 0 >}
```

No configuration data is added here to keep example as simple as possible.