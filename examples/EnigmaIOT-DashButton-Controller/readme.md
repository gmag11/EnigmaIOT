# EnigmaIOT Dash Button Controller

This shows the simplest implementation of [EnigmaIOTjsonController.h](https://github.com/gmag11/EnigmaIOT/blob/dev/src/EnigmaIOTjsonController.h) using deep sleep mode.

A message is sent when node wakes from deep sleep, after triggering RESET button. Then it enters deep sleep mode again for an indefinite time. So reset is the only way to wake the node.

This may be used as a Dash Button like device. Other uses may be a door sensor, a PIR movement detector, a laser barrier or any other sensor that may trigger reset when activated.

Using [EnigmaIOT Gateway MQTT](https://github.com/gmag11/EnigmaIOT/tree/dev/examples/EnigmaIOTGatewayMQTT), message from Dash Button controller produces this output.

```
<Network_name>/<node_name or node_address>/data  {"button":1}
```

No configuration data is added here to keep example as simple as possible.