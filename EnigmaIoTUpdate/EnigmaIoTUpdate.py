import base64
import paho.mqtt.client as mqtt
import time
import hashlib
import optparse
import os

# EnigmaIoTUpdate -f <file.bin> -d <address> -t <basetopic> -u <mqttuser> -P <mqttpass> -s <mqttserver>
#                       -p <mqttport> <-s> -D <speed>

options = None
sleepyNode = True
resultTopic = "/result/#"
sleepSetTopic = "/set/sleeptime"
sleepResultTopic = "/result/sleeptime"
otaSetTopic = "/set/ota"
otaResultTopic = "/result/ota"
otaOutOfSequenceError = "OTA out of sequence error"
otaOK = "OTA finished OK"
# otaLength = 0
otaFinished = False
idx = 0
packetDelay = 0.035


def on_connect(client, userdata, flags, rc):
    global options

    if rc == 0:
        print("Connected with result code "+str(rc))
        mqtt.Client.connected_flag = True
    else:
        print("Error connecting. Code ="+str(rc))
        return

    sleep_topic = options.baseTopic+"/"+options.address+resultTopic
    client.subscribe(sleep_topic)
    print("Subscribed")


def on_message(client, userdata, msg):
    global sleepyNode
    global idx, otaFinished

    if msg.topic.find(sleepResultTopic) >= 0 and msg.payload == b'0':
        sleepyNode = False
        print(msg.topic + " " + str(msg.payload))

    payload = msg.payload.decode ('utf-8')

    if msg.topic.find(otaResultTopic) >= 0:

        if payload.find(otaOutOfSequenceError) >= 0:
            print(payload.split(',')[0],end='')
            idx = int(payload.split(',')[0])

        elif payload.find(otaOK) >= 0:
            print(" OTA Finished ", end='')
            otaFinished = True


def main():
    global options
    global sleepyNode
    global otaFinished
    global packetDelay

    opt = optparse.OptionParser()
    opt.add_option("-f", "--file",
                   type="string",
                   dest="filename",
                   default="program.bin",
                   help="File to program into device")
    opt.add_option("-d", "--daddress",
                   type="string",
                   dest="address",
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
    opt.add_option("-D", "--speed",
                   type="string",
                   dest="otaSpeed",
                   default="fast",
                   help="OTA update speed profile: 'fast', 'medium' or 'slow'. Default: %default")

    (options, args) = opt.parse_args()

    if not options.address:
        opt.error('Destination address not supplied')

    # print(options)

    ota_topic = options.baseTopic+"/"+options.address+otaSetTopic
    mqttclientname = "EnigmaIoTUpdate"

    ota_length = os.stat(options.filename).st_size

    if options.otaSpeed == "medium":
        packetDelay = 0.07
    elif options.otaSpeed == "slow":
        packetDelay = 0.14

    with open(options.filename, "rb") as binary_file:
        chunked_file = []
        encoded_string = []
        n = 212  # Max 217 - 2. Divisible by 4 => 212

        for chunk in iter(lambda: binary_file.read(n), b""):
            chunked_file.append(chunk)
        for chunk in chunked_file:
            encoded_string.append(base64.b64encode(bytes(chunk)).decode('ascii'))
        # chunked_string = [encoded_string[i:i+n] for i in range(0, len(encoded_string), n)]
        binary_file.seek(0);
        hash_md5 = hashlib.md5()
        for chunk in iter(lambda: binary_file.read(4096), b""):
            hash_md5.update(chunk)

        # print(hash_md5.hexdigest())
        binary_file.close()

    mqtt.Client.connected_flag = False
    client = mqtt.Client(mqttclientname, True)
    client.username_pw_set(username=options.mqttUser, password=options.mqttPass)
    if options.mqttSecure:
        client.tls_set()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(host=options.mqttServer, port=options.mqttPort)
    while not client.connected_flag:  # wait in loop
        print("Connecting to MQTT server")
        client.loop()
        time.sleep(1)

    # client.loop_start()
    sleep_topic = options.baseTopic+"/"+options.address+sleepSetTopic
    client.publish(sleep_topic, "0")

    while sleepyNode:
        print("Waiting for non sleepy confirmation")
        client.loop()
        time.sleep(1)

    print("Sending hash: "+hash_md5.hexdigest())
    md5_str = hash_md5.hexdigest()

    # msg 0, file size, number of chunks, md5 checksum
    client.publish(ota_topic, "0," + str(ota_length) + "," + str(len(encoded_string)) + "," + md5_str)

    # for i in range(0, len(chunked_string), 1):
    print("Sending file: "+options.filename)
    global idx
    while idx < len(encoded_string):
        client.loop()
        time.sleep(packetDelay)
        # time.sleep(0.2)
        # if i not in range(10,13):
        i = idx + 1
        client.publish(ota_topic, str(i)+","+encoded_string[idx])
        idx = idx + 1
        if i % 2 == 0:
            print(".", end='')
        if i % 160 == 0:
            print(" %.f%%" % (i/len(encoded_string)*100))
        if i == len(encoded_string):
            for i in range(0, 40):
                client.loop()
                time.sleep(0.5)
                if otaFinished:
                    print(" OTA OK ", end='')
                    break

    print("100%")
    # time.sleep(5)
    client.disconnect()


if __name__ == '__main__':
    main()
