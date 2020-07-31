# EnigmaIOT Node example

This is a basic and straightforward example of an EnigmaIOT node. It only sends a message with mocked values, in addition to input voltage value on ESP8266 and ESP32, and sleeps during 10 seconds.

If you use an ESP8266 you have to connect GPIO16 and RST pins or it will never wake from sleep. You can reset your ESP8266 board manually to force a wake.

It uses CayenneLPP as payload encoding. CayenneLPP is a pretty efficient encoding format. Other examples uses MessagePack that is less efficient but much more versatile. So I recommend using the latter unless you have many values to send that do not fin in the maximum 214 bytes of a single EnigmaIOT message.

In order to adapt it to your needs you only have to modify this code to readout sensor values.

```c++
// User code

CayenneLPP msg (MAX_DATA_PAYLOAD_SIZE);

msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
msg.addTemperature (1, 20.34);
msg.addDigitalInput (2, 123);
msg.addBarometricPressure (3, 1007.25);
msg.addCurrent (4, 2.43);

// End of user code

// Send buffer data
EnigmaIOTNode.sendData (msg.getBuffer (), msg.getSize ());
```

When receiving this message, MQTT Gateway produces this output:

```json
<network_name>/<node_name>/data 

[{"channel":0, "type":2,   "name":"analog_input",  "value":3.28},
 {"channel":1, "type":103, "name":"temperature",   "value":20.3},
 {"channel":2, "type":0,   "name":"digital_input", "value":123},
 {"channel":3, "type":115, "name":"pressure",      "value":1007.2},
 {"channel":4, "type":117, "name":"current",       "value":2.43}]
```

