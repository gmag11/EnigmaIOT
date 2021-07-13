## OTA Update

MQTT Gateway example includes plain Arduino OTA mechanism. OTA is protected using network key selected during initial configuration.

On nodes, OTA updates are transferred using the same mechanism. Firmware is sent over MQTT using a [Python script](./EnigmaIoTUpdate/EnigmaIoTUpdate.py). Then gateway selects the appropriate node and send this binary data over ESP-NOW.

As ESP-NOW restricts **maximum payload to 250 bytes per message** firmware is splitted in chunks. Every chunk is **212 bytes** long, so that it fits together with message headers and is multiple of 4. This splitting work is done by `EnigmaIoTUpdate.py` script.

### Using EnigmaIoTUpdate.py

A requirement is to have installed [Python3](https://www.python.org/download/releases/3.0/) in the computer used to do the update.

In order to run the update, you need to install [`paho-mqtt`](https://pypi.org/project/paho-mqtt/) library. To do that you can follow instructions [here](https://pypi.org/project/paho-mqtt/#installation).

```
$python3 ./EnigmaIoTUpdate.py --help

Usage: EnigmaIoTUpdate.py [options]

Options:
  -h, --help            show this help message and exit
  -f FILENAME, --file=FILENAME
                        File to program into device
  -d ADDRESS, --daddress=ADDRESS
                        Node address or name
  -t BASETOPIC, --topic=BASETOPIC
                        Base topic for MQTT messages
  -u MQTTUSER, --user=MQTTUSER
                        MQTT server username
  -P MQTTPASS, --password=MQTTPASS
                        MQTT server user password
  -S MQTTSERVER, --server=MQTTSERVER
                        MQTT server address or name
  -p MQTTPORT, --port=MQTTPORT
                        MQTT server port
  -s, --secure          Use secure TLS in MQTT connection. Normally you should
                          use port 8883
  -D, --speed			Sets formware delivery speed [fast | medium | slow]. The fastest
                          the biggest chance to get troubles during update. Fast option
                          normally works but medium is more resilient
  --unsecure            Use secure plain TCP in MQTT connection. Normally you
                          should use port 1883
```

An example of this command could be like this:

```
python3 ./EnigmaIoTUpdate.py \
             -f EnigmaIOTsensor.bin \
             -d 11:22:33:44:55:66 \
             -t enigmaiot \
             -u "mymqttbrokeruser" \
             -P "mymqttbrokerpassword" \
             -S mysecure.mqtt.server \
             -p 8883 \
             -D medium \
             -s
```

Notice that using ESP-NOW, device address correspond to **MAC address** of your ESP8266 or ESP32 node.

It is very important to configure user and password on you MQTT broker. Besides, if it is going to be accessed from the Internet you should activate TLS encryption and a certificate.

