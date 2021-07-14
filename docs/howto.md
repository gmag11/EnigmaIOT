This document will serve as a guide to start working with EnigmaIOT, enabling you to develop your own secure sensor network easily, with a few additional lines of code compared with a regular Arduino program.

# What you need

### Gateway
Any ESP32 or ESP8266 will do the job. Anyway, it is always recommended to use an **ESP32** board because having much more RAM it will be more stable along time.

**Notice that it is not possible to configure any node if you don't have a gateway working nearby.**

### MQTT broker
You need to use a MQTT broker (or server). Any public or private will do the job. As EnigmaIOT is focused on privacy I advise to install your own MQTT private broker. You can do it on any home server or Raspberry Pi, or even in a virtual private server.
Installing and configuring a broker is out of scope of this guide but there are plenty of good and easy guides online.

A good choice for a MQTT broker is [Eclipse Mosquitto](https://mosquitto.org).

Don't forget to add a user and password to broker at least. EnigmaIOT supports MQTT brokers with TLS encryption. If you expose your broker to the public Internet adding TLS to your setup will improve privacy and security, so it is highly encouraged.

Using MQTT enables you to use a wide range or solutions to process, display information and manage your EnigmaIOT nodes. Good choices are [Node-Red](https://nodered.org) and [Home Assistant](https://www.home-assistant.io), although you can use any software that is able to communicate with a MQTT broker, or any combination of them.

## Required External libraries

All examples have a `platformio.ini` file so that they can be compiled using PlatformIO without any additional requirement.

If you use Arduino IDE instead, you have to install all these libraries into your environment:

- ESPAsyncTCP -- https://github.com/me-no-dev/ESPAsyncTCP **(Required only for ESP8266)**
- AsyncTCP -- https://github.com/me-no-dev/AsyncTCP **(Required only for ESP32)**
- ESPAsyncWebServer -- https://github.com/me-no-dev/ESPAsyncWebServer
- ESPAsyncWiFiManager -- https://github.com/gmag11/ESPAsyncWiFiManager forked from https://github.com/alanswx/ESPAsyncWiFiManager
- Arduino Crypto Library -- https://github.com/gmag11/CryptoArduino forked and formatted from https://github.com/rweather
- PubSubClient -- https://github.com/knolleary/pubsubclient
- CayenneLPP -- https://github.com/sabas1080/CayenneLPP version > 1.0.2
- ArduinoJSON 6 -- https://github.com/bblanchon/ArduinoJson version > 6.0.0

# How to start with EnigmaIOT MQTT Gateway

### Install Gateway code on ESP32 or ESP8266

Code for gateway is included as an example on EnigmaIOT repository, as `EnigmaIOTGatewayMQTT`. You can find it [here](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOTGatewayMQTT).

It may be used as it is. There is no need to modify it to be able to customize to your needs. Everything is done during first configuration.

You can use binary file included in release as an attachment or compile it by yourself.

### Loading binary file

You will need [Espressif esptool](https://github.com/espressif/esptool) utility to flash binary file on your ESP32 or ESP8266. If your system has Python and pip installed you can install esptool by running

```bash
pip install esptool
```

Esptool will detect your board type and the port on which it is attached to so command line will be as simple as this:

It your MQTT broker uses TLS

```bash
python esptool.py write_flash 0 EnigmaIOT-Gateway-ESP32-SSL_MQTT.bin
```

If your MQTT broker does not use encrypted communication

```bash
python esptool.py write_flash 0 EnigmaIOT-Gateway-ESP32-Plain_MQTT.bin
```

If you have a ESP8266 (MQTT encryption is not supported)

```bash
python esptool.py write_flash 0 EnigmaIOT-Gateway-ESP8266.bin
```

### Configuring your gateway

First time you run EnigmaIOT gateway it will behave as a WiFi access point with name `EnigmaIOTGateway`. Connect your smartphone or computer to it.

A web browser should be open automatically pointing to configuration portal. If it is not the case you can access it opening http://192.168.4.1.

You should see something like this:

<img src="https://raw.githubusercontent.com/gmag11/EnigmaIOT/master/img/EnigmaIOTGateway_config_portal.png" style="zoom:50%;" />



Click on `Configure WiFi` and board will scan networks. Select yours and continue filling all fields



<img src="https://raw.githubusercontent.com/gmag11/EnigmaIOT/master/img/EnigmaIOTGateway_wifi_selection.png" alt="image-20210703000533470" style="zoom: 50%;" />

Fields explanation:

**SSID**: Name of your home WiFi network.

**WiFi Password**: Your home WiFi password.

**Network Key**: Network key for your EnigmaIOT network. All nodes and gateway must share this key. Choose a secure password and keep it safe.

**Network Channel**: EnigmaIOT gateway initial channel. This is not relevant for MQTT gateway as it will use the same as your WiFi network.

**Network Name**: EnigmaIOT network name. This identify your network. This name will be used as root for all MQTT messages to and from this gateway.

**MQTT broker address**: IP address or hostname where MQTT broker is listening on

**TCP port**: TCP port used by MQTT broker. 8883 is normally used by brokers with TLS encryption configured. 1883 is used otherwise.

**MQTT username**: Username configured on MQTT broker.

**MQTT password**: User password.

When you click `save` button and boards successfully connects to your WiFi network it reboots to start working as a real EnigmaIOT gateway.

Now you are ready to start your own EnigmaIOT network.

### Customizing gateway firmware

Although you can use gateway code as it is. There are some customizations available for experienced users.

#### EnigmaIOTGatewayMQTT.ino

`LED_BUILTIN`: On many ESP32 boards, built in LED is connected to Pin 5. On EnigmaIOTGatewayMQTT, LED is used to signal configuration mode (flashing LED) and to show activity from nodes. If your board uses a different pin you can set it here. You may use different LEDs to signal received or sent message, In that case you may set `BLUE_LED` and `RED_LED`.

#### dstrootca.h

If you use TLS to encrypt communication with MQTT broker, then you must provide the root certificate used to check server certificate. This varies between different certificate providers. Included certificate works with  [Let's encrypt](https://letsencrypt.org). If you use a different provider or you have setup your own public key infrastructure you need to copy your CA certificate in [PEM format](https://knowledge.digicert.com/quovadis/ssl-certificates/ssl-general-topics/what-is-pem-format.html) assigned to `DSTroot_CA` variable.

You can use [certificate decoder from SSLChecker.com](https://www.sslchecker.com/certdecoder) to dump certificate data, as expiration date.

#### EnigmaIOTconfig.h

This file is on library code directory. There are some settings that you can tweak there before compilation. There are explanation for every parameter on file itself. Although it is safe to adjust these settings some combinations may lead your gateway to not be able to communicate with nodes. I do not recommend tweaking settings that you don't understand clearly.

`NUM_NODES` parameter is used to configure the maximum number of nodes. It is set to 35 by default that should be enough for most users. You may increase it if you expect your network to grow above this limit. 

For ESP32 boards a limit of 100 or 200 nodes is safe, but ESP8266 may not have enough memory so if you find frequent reboots after setting this restore it to the default value.

# How to develop a node with EnigmaIOT on ESP32 or ESP8266

EnigmaIOT is designed to hide all complexity behind, so that anyone that is barely familiar with Arduino environment is able to develop a node.

### Bare basic code

Simplest node code may be something like this.

```c++
#include <EnigmaIOTNode.h> // Include EnigmaIOT node library
#include <espnow_hal.h>    // Add ESP-NOW subsystem
void setup () {
    EnigmaIOTNode.begin (&Espnow_hal); // Start node code
    char msg[] = "20"; // Build a message to send
    EnigmaIOTNode.sendData ((uint8_t*)msg, sizeof (msg) - 1, RAW); // Send data as RAW
    EnigmaIOTNode.sleep (); // Request node to sleep
}
void loop () {
    EnigmaIOTNode.handle (); // Don't forget this
}
```

If you use [PlatformIO](https://platformio.org) IDE you may use this platformio.ini

```ini
[env:d1_mini_pro]
platform = espressif8266
board = d1_mini_pro
framework = arduino
lib_deps =
   ESPAsyncWiFiManager
   ESP Async WebServer
   ArduinoJson
   https://github.com/gmag11/CryptoArduino.git
   https://github.com/gmag11/EnigmaIOT.git
```

### First node configuration

In the same way as we did with gateway, when node starts for first time it announces itself as a WiFi access point with name EnigmaIoTNode followed by chipID.

When you connect to this AP without password you get a web page like this:

<img src="https://raw.githubusercontent.com/gmag11/EnigmaIOT/img/EnigmaIOTNode_config_portal.png" alt="image-20210704121322147" style="zoom:50%;" />

Click on `Configure WiFi` button and you will get the list of WiFi networks around. It is important not to select your home WiFi network here. It is not what an EnigmaIOT node needs. You should select the one whose name is you EnigmaIOT network name. This comes from your EnigmaIOT gateway.

This AP needs a password it is the one that you configured as Network key in your gateway.

<img src="https://raw.githubusercontent.com/gmag11/EnigmaIOT/master/img/EnigmaIOTNode_wifi_selection.png" alt="image-20210704121626137" style="zoom:50%;" />



You need to fill this settings in:

**SSID**: Your network name (as configured in gateway)

**Password**: Your network key (as configured in gateway)

**Sleep time**: A node may be designed to sleep after sending a message. In that case this is the default sleep time in seconds. If your node does not go to sleep mode, then this setting is ignored.

**Node Name**: This is your node name. It must be unique in your network. It a gateway find a node with duplicate name this name will be ignored and will use its MAC address instead.

------------

After information is saved, node checks that it can connect Gateway WiFi AP successfully. If so, it reboots and start sending data.

Using EnigmaIOTGatewayMQTT you will get a MQTT message every time your node sends data.

```
EnigmaIOT/SimpleNode/data   20
```

Topic format is always the same:

```
<NetworkName>/<NodeName>/data
```

# Developing advanced nodes using JSONController template

If you need to build a node you may start coding from scratch as it is shown before. Additionally you can use a Template so many features are already implemented transparently:

- Sleep management
- Connection and disconnection handling
- Send data as JSON object
- [Home Assistant](https://www.home-assistant.io) auto discovery integration feature.
- WiFi Manager custom parameters
- Integrated [fail safe mode](https://github.com/gmag11/FailSafeMode)

### Designing sensor algorithm

JSON Controller wraps EnigmaIOT node with additional features. It is implemented as a cpp and h files that contains `setup ()` and `loop ()` functions. You should use them instead main setup and loop.

First step I recommend is coding a simple sketch that deals with your hardware (sensors, actuators) as a regular Arduino program. To illustrate this I will use EnigmaIOT-Sensor-Controller example. It is a node that uses a DS18B20 temperature sensor that reads temperature value, sends it and then sleeps until next measurement.

So starting code could look like this

```c++
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4 // I/O pin used to communicate with sensor

//----------------- GLOBAL VARIABLES -----------------
OneWire* oneWire;
DallasTemperature* sensors;
DeviceAddress insideThermometer;
bool tempSent = false; // True when
float tempC;
//----------------- GLOBAL VARIABLES -----------------

bool sendTemperature (float temp) { // Mock function. This will later be adapted to send an EnigmaIOT message
    Serial.printf ("Temperarure: %f\n", temp);
    return true;
}

void setup () {
    Serial.begin (115200); // Only for this test
    
    //----------------- USER CODE -----------------
    oneWire = new OneWire (ONE_WIRE_BUS);
	sensors = new DallasTemperature (oneWire);
	sensors->begin ();
	sensors->setWaitForConversion (false);
	sensors->requestTemperatures ();
    
    time_t start = millis (); 
    
    while (!sensors->isConversionComplete ()) {
		delay (0);
	}
	Serial.printf ("Conversion completed in %lld ms\n", millis () - start);
    tempC = sensors->getTempCByIndex (0);
    //----------------- USER CODE END -------------
}

void loop () {
    //----------------- USER CODE -----------------
    if (!tempSent) { 
        if (sendTemperature (tempC)) { // First time this will be executed
            tempSent = true;
        }
    }
    //----------------- USER CODE END -------------
    else {
        ESP.deepSleep(10000); // Next time deep sleep mode will be requested automatically. Only for testing
    }
}
```

When this code has the functionality that you need and it is tested you can follow with integration on EnigmaIOT JSON Controller code.

### Code integration into EnigmaIOT

To develop a new JSONController node you need to use [JSONController template](https://github.com/gmag11/EnigmaIOT/tree/dev/examples/EnigmaIOT-Json-Controller-Template) example as starting point.

I will follow the process to get to the point implemented in [**EnigmaIOT-Sensor-Controller**](https://github.com/gmag11/EnigmaIOT/tree/dev/examples/EnigmaIOT-Sensor-Controller).

You need to follow these steps:

#### Rename JSON Controller files and class

   It is recommended to rename cpp and h files so that its name is coherent with the function they have. Following this, `BasicController.h/cpp`  will become `ds18b20Controller.h/cpp`. Then edit `CONTROLLER_CLASS_NAME` and `CONTROLLER_NAME` on `ds18b20Controller.h` like this:

```c++
#define CONTROLLER_CLASS_NAME ds18b20Controller
static const char* CONTROLLER_NAME = "DS18B20 controller";
```

and two first lines to:

```c++
#ifndef _DS18B20CONTROLLER_h
#define _DS18B20CONTROLLER_h
```

#### Define if your node should sleep

If you want to design a node that is powered with batteries, then it should enter into deep sleep mode after sending its data. To do so you only need to set `SLEEPY ` to 1 or `true`. You need to do so on main cpp file. In sensor controller example, it is `EnigmaIOT-Sensor-Controller.ino`.

```c++
#define SLEEPY 1 // Set it to 1 if your node should sleep after sending data
```

It you don't need sleep function leave it as 0.

Notice that non sleepy nodes have an additional time synchronization function that is not available for nodes that enter deep sleep mode. This allows you to add features as timer or time synchronized tasks in different nodes.

#### Copy global variables as class parameters

You need to add all global variables defined in Arduino code as JSON Controller class parameters in `ds18b20Controller.h`.

```c++
class CONTROLLER_CLASS_NAME : EnigmaIOTjsonController {
protected:
	// --------------------------------------------------
	// add all parameters that your project needs here
	// --------------------------------------------------
	OneWire* oneWire;
	DallasTemperature* sensors;
    DeviceAddress insideThermometer;
    bool tempSent = false;
    float tempC;
```

#### Add custom libraries

If your code needs custom libraries you may add them on JSON controller header file (`ds18b20Controller.h`)

```c++
// --------------------------------------------------
// You may define data structures and constants here
// --------------------------------------------------
#include <DallasTemperature.h>
```



#### Add custom function as class methods

In the same way you should add custom functions like `sendTemperature ()` as class methods. You need to define them in `ds18b20Controller.h`

```c++
    // ------------------------------------------------------------
	// You may add additional method definitions that you need here
	// ------------------------------------------------------------

    bool sendTemperature (float temp);
```

#### Include Home Assistant integration

If you like to integrate your node into HomeAssistant you may include the corresponding header file.

```c++
#if SUPPORT_HA_DISCOVERY    
#include <haSensor.h>
#endif
```

You need to choose the file to include according node function. As this will be a sensor node we should use `haSensor.h`. If your node uses different profiles you can include several HA integration header files. For instance, a smart metering plug is a sensor (Power measurement) and binary switch (ON-OFF).

Then you need to add configuration on cpp file. In sensor controller node example it is 

```c++
	// *******************************
    // Add your characteristics here
    // There is no need to futher modify this function
    haEntity->setNameSufix ("temp");
    haEntity->setDeviceClass (sensor_temperature);
    haEntity->setExpireTime (3600);
    haEntity->setUnitOfMeasurement ("ºC");
    haEntity->setValueField ("temp");
    // *******************************
```

#### Add defines

All needed defines and constants that you need in your code may be added at the beginning of controller cpp file. You can add them to header file instead but it is a good practice to restrict its code visibility.

```c++
// -----------------------------------------
// You may add some global variables you need here,
// like serial port instances, I2C, etc
// -----------------------------------------

#define ONE_WIRE_BUS 4
```

#### Correct JSON controller header include in cpp file

If you modified file and class names you will need to update include in cpp file

```c++
#include "ds18b20Controller.h"
```

#### Add `setup()` and `loop()`code to class method

Now, you can start integrating your code into JSON controller class. You need to copy setup and loop code into corresponding methods in JSON controller class.

```c++
void CONTROLLER_CLASS_NAME::setup (EnigmaIOTNodeClass* node, void* data) {
	enigmaIotNode = node;

    // Send a 'hello' message when initalizing is finished for non sleepy nodes
    if (!enigmaIotNode->getNode ()->getSleepy ()) {
        if (!(enigmaIotNode->getNode ()->getSleepy ())) {
            sendStartAnouncement ();  // Disable this if node is sleepy
        }
    }
    // Send hello end
    
	// You do node setup here. Use it as it was the normal setup() Arduino function

	oneWire = new OneWire (ONE_WIRE_BUS);
	sensors = new DallasTemperature (oneWire);
	sensors->begin ();
	sensors->setWaitForConversion (false);
	sensors->requestTemperatures ();
    time_t start = millis ();

	while (!sensors->isConversionComplete ()) {
		delay (0);
	}
	DEBUG_WARN ("Conversion completed in %d ms", millis () - start);
    tempC = sensors->getTempCByIndex (0);
}
```

```c++
void CONTROLLER_CLASS_NAME::loop () {

	// If your node stays allways awake do your periodic task here

	// You can send your data as JSON. This is a basic example

    if (!tempSent && enigmaIotNode->isRegistered()) {
        if (sendTemperature (tempC)) {
            tempSent = true;
        }
    }
    
}
```

Notice that I've added `enigmaIotNode->isRegistered()` to send data only if node has already registered with Gateway and not losing messages.

#### Add custom functions as class methods

Add every custom function you have used as method into the class. You already added definitions in header file before. Now add the implementation to cpp file

```c++
bool CONTROLLER_CLASS_NAME::sendTemperature (float temp) {
	const size_t capacity = JSON_OBJECT_SIZE (2);
	DynamicJsonDocument json (capacity);
	json["temp"] = temp;

	return sendJson (json);
}
```

This creates a JSON object with all data you need and sends it to gateway using `sendJson ()` method. Notice that EnigmaIOT uses esp-now protocol to communicate nodes with gateway. This implies a limit of 250 bytes per message, including headers. So, if you have problem receiving messages or you get partial data check the length of your payload.

#### Additional functions

There are a few of additional functions. Check other controller examples to learn how to use them.

- [**EnigmaIOTButtonController**](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOT-Button-Controller): Node that send messages when a button is pressed. (Non sleepy)
- [**EnigmaIOT-DashButton-Controller**](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOT-DashButton-Controller): Node that wakes from deep sleep when a button is pressed, send its message and sleeps indefinitely. (Sleepy)
- [**EnigmaIOT-Led-Controller**](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOT-Led-Controller): Node that controls a singled light or LED (Non sleepy)
- [**EnigmaIOT-Sensor-Controller**](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOT-Sensor-Controller): Node that send value from a DS18B20 temperature sensor regularly. (Sleepy)
- [**EnigmaIOT-SmartSwitch-Controller**](https://github.com/gmag11/EnigmaIOT/tree/master/examples/EnigmaIOT-SmartSwitch-Controller): Smart switch that uses a button to toggle a relay. It sends status messages regularly and on every toggle action. It listens for messages to allow remote control. (Non sleepy)

##### Listen for incoming messages from gateway

Some kind of nodes as light controllers or smart switches should accept incoming messages to control different parameters. This can be achieved with `processRxCommand` and `sendCommandResp`.

##### Notice when a node is connected or disconnected from EnigmaIOT network

It may be useful for a node to know if it is actually connected to gateway. To implement this you may fill these two methods: `connectInform` and `disconnectInform`.

##### Save and recover custom persistent configuration

Sometimes you need some data to be stored persistently on node sleeps or power cycles. There are a couple methods that may be implemented to achieve this: `loadConfig` and `saveConfig`.

##### Add custom parameters to configuration portal

You may want to add your own configuration fields to first configuration web portal on node. You can get this by implementing `configManagerStart` and `configManagerExit`.

#### Advanced tuning

There are some advanced settings on `EnigmaIOTconfig.h`. You may modify this data but it is important that you understand what every setting means. If you adjust them randomly you may get into instabilities or your node may be unable to communicate at all. 



