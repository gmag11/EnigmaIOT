import base64
import paho.mqtt.client as mqtt
import time
import hashlib
import optparse

# EnigmaIoTUpdate -f <file.bin> -d <address> -t <basetopic> -u <mqttuser> -P <mqttpass> -s <mqttserver>
#                       -p <mqttport> <-s>


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected with result code "+str(rc))
        mqtt.Client.connected_flag = True
    else:
        print("Error connecting. Code ="+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    #client.subscribe("")


def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

def main():
    opt = optparse.OptionParser()
    opt.add_option("-f", "--file",
                   type="string",
                   dest="filename",
                   default="program.bin",
                   help="File to program into device")
    opt.add_option("-d", "--daddress",
                   type="string",
                   dest="address",
                   default="00:00:00:00:00:00",
                   help="Device address")
    opt.add_option("-t", "--topic",
                   type="string",
                   dest="baseTopic",
                   default="enigmaiot",
                   help="Base topic for MQTT messages")
    opt.add_option("-u", "--user",
                   type="string",
                   dest="mqttUser",
                   default="",
                   help="MQTT server username")
    opt.add_option("-P", "--password",
                   type="string",
                   dest="mqttPass",
                   default="",
                   help="MQTT server user password")
    opt.add_option("-S", "--server",
                   type="string",
                   dest="mqttServer",
                   default="127.0.0.1",
                   help="MQTT server address or name")
    opt.add_option("-p", "--port",
                   type="int",
                   dest="mqttPort",
                   default=1883,
                   help="MQTT server port")
    opt.add_option("-s", "--secure",
                   action="store_true",
                   dest="mqttSecure",
                   help="Use secure TLS in MQTT connection. Normally you should use port 8883")
    opt.add_option("--unsecure",
                   action="store_false",
                   dest="mqttSecure",
                   default=False,
                   help="Use secure plain TCP in MQTT connection. Normally you should use port 1883")

    (options, args) = opt.parse_args()

    print(options)

    topic = options.baseTopic+"/"+options.address+"/ota"
    topicmd5 = options.baseTopic+"/"+options.address+"/ota/md5"
    mqttclientname = "EnigmaIoTUpdate"

    with open(options.filename, "rb") as binary_file:
        chunked_file = []
        encoded_string = []
        n = 200

        for chunk in iter(lambda: binary_file.read(n), b""):
            chunked_file.append(chunk)
        for chunk in chunked_file:
            encoded_string.append(base64.b64encode(chunk).decode("utf8"))
        # chunked_string = [encoded_string[i:i+n] for i in range(0, len(encoded_string), n)]
        binary_file.seek(0);
        hash_md5 = hashlib.md5()
        for chunk in iter(lambda: binary_file.read(4096), b""):
            hash_md5.update(chunk)

        print(hash_md5.hexdigest())
        binary_file.close()

    mqtt.Client.connected_flag = False
    client = mqtt.Client(mqttclientname, True)
    client.username_pw_set(username=options.mqttUser, password=options.mqttPass)
    if options.mqttSecure:
        client.tls_set()
    client.on_connect = on_connect
    client.on_message = on_message
    client.loop_start()

    client.connect(host=options.mqttServer, port=options.mqttPort)
    while not client.connected_flag:  # wait in loop
        print("In wait loop")
        time.sleep(1)

    # Blocking call that processes network traffic, dispatches callbacks and
    # handles reconnecting.
    # Other loop*() functions are available that give a threaded interface and a
    # manual interface.

    client.publish(topicmd5, hash_md5.hexdigest())

    i = 0
    # for i in range(0, len(chunked_string), 1):
    for chunk in encoded_string:
        # client.loop()
        time.sleep(0.03)
        # time.sleep(0.003125)
        client.publish(topic, str(i)+","+chunk)
        i = i + 1


if __name__ == '__main__':
    main()