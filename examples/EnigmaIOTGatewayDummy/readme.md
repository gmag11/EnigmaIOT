# EnigmaIOT Gateway Dummy

This example represent the basis to develop an EnigmaIOT gateway. You may build a gateway that register measurements from node sensors to an SD or uses LoRaWAN as output instead MQTT.

To do that you only need to implement the class defined in [GwOutput_generic.h](https://github.com/gmag11/EnigmaIOT/blob/master/src/GwOutput_generic.h)

This only implements the EnigmaIOT side using ESP-NOW.