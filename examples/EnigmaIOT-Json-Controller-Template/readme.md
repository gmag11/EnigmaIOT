# EnigmaIOT JSON Controller Template

This adds a layer to EnigmaIOT node using the concept of **controller**.

A controller is a class that gets data and execute commands in peripherals of a node. Those peripherals may be sensors, buttons, relays, leds or any other hardware that you may use in a microcontroller hardware.

EnigmaIOT library includes an interface definition in [EnigmaIOTjsonController.h](https://github.com/gmag11/EnigmaIOT/blob/dev/src/EnigmaIOTjsonController.h) that you need to implement in order to include EnigmaIOT communication to a simple project.

This example may be used as a template to start to develop any kind of node. It allows sending and receiving data hiding the complexity of layers below.

If your code needs configuration parameters that have to persist reboots, interface gives a simple way to add those parameters to EnigmaIOT node web configuration interface. Check other controller examples to see how this may be used.

This is called JSON controller because all user payload is encoded using JSON objects, so this makes that Gateway transfers data transparently to/from the right peer.