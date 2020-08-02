# EnigmaIOT Button Controller

This shows the simplest implementation of [EnigmaIOTjsonController.h](https://github.com/gmag11/EnigmaIOT/blob/dev/src/EnigmaIOTjsonController.h) using a button as data origin.

A message is sent on button press and another on button release. Node does not enter in deep sleep mode.

Simple debounce is applied to avoid multiple pushes.

Using [EnigmaIOT Gateway MQTT](https://github.com/gmag11/EnigmaIOT/tree/dev/examples/EnigmaIOTGatewayMQTT), messages from button controller produce this output.

```
<Network_name>/<node_name or node_address>/data  {"button":<button_pin>,"push":< 1 or 0 >}
```

- 1 means button pushed
- 0 means button released

No configuration data is added here to keep example as simple as possible.