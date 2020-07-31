# EnigmaIOT MsgPack example

This is the equivalent of EnigmaIOT node example but shows how to use JSON and MsgPack to encode data before transmission. It sends a message with mocked values, in addition to input voltage value on ESP8266 and ESP32, and sleeps during 10 seconds.

If you use an ESP8266 you have to connect GPIO16 and RST pins or it will never wake from sleep. You can reset your ESP8266 board manually to force a wake.

In order to adapt it to your needs you only have to modify this code to readout sensor values.

```c++
// Put here your code to read sensor and compose buffer
const size_t capacity = JSON_OBJECT_SIZE (5); // Adapt capacity to your needs. See https://arduinojson.org/v6/assistant/ for guidance
DynamicJsonDocument json (capacity);

json["V"] = (float)(ESP.getVcc ()) / 1000;
json["tem"] = 203;
json["din"] = 123;
json["pres"] = 1007;
json["curr"] = 2.43;

int len = MAX_DATA_PAYLOAD_SIZE;
uint8_t buffer[MAX_DATA_PAYLOAD_SIZE];
len = serializeMsgPack (json, (char*)buffer, len);
// End of user code

// Send buffer data
EnigmaIOTNode.sendData (buffer, len, MSG_PACK);
```

When receiving this message, MQTT Gateway produces this output:

```json
<network_name>/<node_name>/data 

{ "V":3.003, "tem":203, "din":123, "pres":1007, "curr":2.43 }
```

This nodes allows receiving downlink messages sending MQTT messages to gateway as

```json
<network_name>/<node_name>/set/data <user data>
```

If user data is given in JSON format, EnigmaIOT gateway will convert it to MsgPack format automatically.

Node will dump its content on serial port.