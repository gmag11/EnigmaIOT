# EnigmaIOT Sensor Controller

EnigmaIOT Sensor Controller example shows how to create a node that reads a value from a sensor, sends its value and sleeps for a certain time.

Sensor used is a DS18B20 thermometer, connected to pin 4.

Output messages using EnigmaIOT Gateway MQTT is like this:

```
<Network_name>/<node_name or node_address>/data  {"temp":<temperature in celsius>}
```

