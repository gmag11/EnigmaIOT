import base64
import paho.mqtt.client as mqtt
import time
import hashlib
import argparse
import os
import json

# EnigmaIoTUpdate -f <file.bin> -d <address> -t <basetopic> -u <mqttuser> -P <mqttpass> -s <mqttserver>
#                       -p <mqttport> <-s> -D <speed>

args = None
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

OTA_OUT_OF_SEQUENCE = 4
OTA_FINISHED = 6


def on_connect(client, userdata, flags, rc):
    global args

    if rc == 0:
        print("Connected with result code " + str(rc))
        mqtt.Client.connected_flag = True
    else:
        print("Error connecting. Code =" + str(rc))
        return

    sleep_topic = args.baseTopic + "/" + args.address + resultTopic
    client.subscribe(sleep_topic)
    print("Subscribed")


def on_message(client, userdata, msg):
    global sleepyNode
    global idx, otaFinished

    payload = json.loads(msg.payload)

    if msg.topic.find(sleepResultTopic) >= 0 and payload['sleeptime'] == 0:
        sleepyNode = False
        print(msg.topic + " " + str(msg.payload))

    # payload = msg.payload.decode('utf-8')

    if msg.topic.find(otaResultTopic) >= 0:

        if payload['status'] == OTA_OUT_OF_SEQUENCE:
            print(payload['last_chunk'], end='')
            idx = int(payload['last_chunk'])

        elif payload['status'] == OTA_FINISHED:
            print(" OTA Finished ", end='')
            otaFinished = True


def main():
    global args
    global sleepyNode
    global otaFinished

    opt = argparse.ArgumentParser(description='This program allows updating EnigmaIOT node over the air using'
                                              'MQTT messages.')
    opt.add_argument("-f", "--file",
                     type=str,
                     dest="filename",
                     default="program.bin",
                     help="File to program into device")
    opt.add_argument("-d", "--daddress",
                     type=str,
                     dest="address",
                     help="Device address")
    opt.add_argument("-t", "--topic",
                     type=str,
                     dest="baseTopic",
                     default="enigmaiot",
                     help="Base topic for MQTT messages")
    opt.add_argument("-u", "--user",
                     type=str,
                     dest="mqttUser",
                     default="",
                     help="MQTT server username")
    opt.add_argument("-P", "--password",
                     type=str,
                     dest="mqttPass",
                     default="",
                     help="MQTT server user password")
    opt.add_argument("-S", "--server",
                     type=str,
                     dest="mqttServer",
                     default="127.0.0.1",
                     help="MQTT server address or name")
    opt.add_argument("-p", "--port",
                     type=int,
                     dest="mqttPort",
                     default=1883,
                     help="MQTT server port")
    opt.add_argument("-s", "--secure",
                     action="store_true",
                     dest="mqttSecure",
                     help="Use secure TLS in MQTT connection. Normally you should use port 8883")
    opt.add_argument("--unsecure",
                     action="store_false",
                     dest="mqttSecure",
                     default=False,
                     help="Use secure plain TCP in MQTT connection. Normally you should use port 1883")
    opt.add_argument("-D", "--speed",
                     type=str,
                     dest="otaSpeed",
                     default="fast",
                     help="OTA update speed profile: 'fast', 'medium' or 'slow' Throttle this down in case of"
                          "problems with OTA update. Default: %default")

    # (options, args) = opt.parse_args()
    args = opt.parse_args()

    if not args.address:
        opt.error('Destination address not supplied')

    # print(options)

    ota_topic = args.baseTopic + "/" + args.address + otaSetTopic
    mqttclientname = "EnigmaIoTUpdate"

    ota_length = os.stat(args.filename).st_size

    delay_options = {"fast": 0.02, "medium": 0.06, "slow": 0.18}
    packet_delay = delay_options.get(args.otaSpeed, 0.07)

    with open(args.filename, "rb") as binary_file:
        chunked_file = []
        encoded_string = []
        n = 212  # Max 215 - 2. Divisible by 4 => 212

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
    client.username_pw_set(username=args.mqttUser, password=args.mqttPass)
    if args.mqttSecure:
        client.tls_set()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(host=args.mqttServer, port=args.mqttPort)
    while not client.connected_flag:  # wait in loop
        print("Connecting to MQTT server")
        client.loop()
        time.sleep(1)

    # client.loop_start()
    sleep_topic = args.baseTopic + "/" + args.address + sleepSetTopic
    client.publish(sleep_topic, "0")

    while sleepyNode:
        print("Waiting for non sleepy confirmation")
        client.loop()
        time.sleep(1)

    print("Sending hash: " + hash_md5.hexdigest())
    md5_str = hash_md5.hexdigest()

    # msg 0, file size, number of chunks, md5 checksum
    print("Sending %d bytes in %d chunks" % (ota_length,len(encoded_string)))
    client.publish(ota_topic, "0," + str(ota_length) + "," + str(len(encoded_string)) + "," + md5_str)

    # for i in range(0, len(chunked_string), 1):
    print("Sending file: " + args.filename)
    global idx

    # remove to simulate lost message
    # error = False

    while idx < len(encoded_string):
        client.loop()
        time.sleep(packet_delay)
        # time.sleep(0.2)
        # if i not in range(10,13):
        i = idx + 1
        client.publish(ota_topic, str(i) + "," + encoded_string[idx])
        idx = idx + 1

        # remove to simulate lost message
        # if idx == 100 and not error:
        #    error = True
        #    idx = idx + 1

        if i % 2 == 0:
            print(".", end='')
        if i % 160 == 0:
            print(" %.f%%" % (i / len(encoded_string) * 100))
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
