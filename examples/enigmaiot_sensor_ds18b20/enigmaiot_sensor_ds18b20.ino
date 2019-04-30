/**
  * @file enigmaiot_sensor.ino
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief Sensor node based on EnigmaIoT over ESP-NOW
  *
  * Sensor reading code is mocked on this example. You can implement any other code you need for your specific need
  */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <EnigmaIOTSensor.h>
#include <espnow_hal.h>
#include <CayenneLPP.h>
#define BLUE_LED 2
//uint8_t gateway[6] = { 0x5E, 0xCF, 0x7F, 0x80, 0x34, 0x75 };
uint8_t gateway[6] = { 0xBE, 0xDD, 0xC2, 0x24, 0x14, 0x97 };

#define SLEEP_TIME 10000000

ADC_MODE (ADC_VCC);

#define ONE_WIRE_VCC D1
#define ONE_WIRE_BUS D2
OneWire oneWire (ONE_WIRE_BUS);
DeviceAddress thermometer;
DallasTemperature sensors (&oneWire);
const uint8_t resolution = 9;

void connectEventHandler () {
    Serial.println ("Connected");
}

void disconnectEventHandler () {
    Serial.println ("Disconnected");
}

void printAddress (DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16) Serial.print ("0");
        Serial.print (deviceAddress[i], HEX);
    }
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length) {
    char macstr[18];
    mac2str (mac, macstr);
    Serial.println ();
    Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));
    for (int i = 0; i < length; i++) {
        Serial.print ((char)buffer[i]);
    }
    Serial.println ();
    Serial.println ();
}

void enigmaiotInit () {
    EnigmaIOTSensor.setLed (BLUE_LED);
    EnigmaIOTSensor.onConnected (connectEventHandler);
    EnigmaIOTSensor.onDisconnected (disconnectEventHandler);
    EnigmaIOTSensor.onDataRx (processRxData);
    EnigmaIOTSensor.begin (&Espnow_hal, gateway, (uint8_t*)NETWORK_KEY);
}

bool init_ds18b20 () {
    Serial.print ("Locating devices...");
    sensors.begin ();
    sensors.setWaitForConversion (false);
    Serial.print ("Found ");
    Serial.print (sensors.getDeviceCount (), DEC);
    Serial.println (" devices.");
    if (!sensors.getAddress (thermometer, 0)) {
        Serial.println ("Unable to find address for Device 0");
        return false;
    }
    sensors.setResolution (resolution);
    Serial.print ("Device 0 Address: ");
    printAddress (thermometer);
    Serial.println ();
    Serial.print ("Device 0 Resolution: ");
    Serial.print (sensors.getResolution (thermometer), DEC);
    Serial.println ();
    Serial.printf ("Parasite %s\n", sensors.isParasitePowerMode () ? "ON" : "OFF");

    Serial.println ();

    return true;
}

float readTemp (DeviceAddress address) {
    time_t start = millis ();
    sensors.requestTemperatures ();
    //delay (750 / (1 << (12 - resolution)));
    while (!sensors.isConversionComplete ()) {
        delay (1);
    }
    Serial.printf ("Conversion complete: %s\n", sensors.isConversionComplete () ? "YES" : "NO");
    Serial.printf ("Resolution: %d\n", sensors.getResolution ());
    //delay (450);
    Serial.printf ("Measurement done: %d ms\n", millis () - start);
    float tempC = sensors.getTempC (address);
    if (tempC == DEVICE_DISCONNECTED_C)
    {
        Serial.println ("Error: Could not read temperature data");
        return DEVICE_DISCONNECTED_C;
    }
    Serial.print ("Temp C: ");
    Serial.println (tempC);
    return tempC;
}

void setup () {
    time_t start = millis ();

    CayenneLPP msg (20);

    Serial.begin (115200); Serial.println (); Serial.println ();

    pinMode (ONE_WIRE_VCC, OUTPUT);
    digitalWrite (ONE_WIRE_VCC, HIGH);

    init_ds18b20 ();
    float temperature = readTemp (thermometer);
    digitalWrite (ONE_WIRE_VCC, LOW);

    enigmaiotInit ();

    msg.addAnalogInput (0, (float)(ESP.getVcc ())/1000);
    Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
    msg.addTemperature (1, temperature);
    //char *message = "Hello World!!!";

    Serial.printf ("Trying to send: %s\n", printHexBuffer (msg.getBuffer (), msg.getSize ()));

    EnigmaIOTSensor.sendData (msg.getBuffer (), msg.getSize ());

    EnigmaIOTSensor.sleep (SLEEP_TIME);

    Serial.printf ("Total time: %d ms\n", millis () - start);
}

void loop () {

    EnigmaIOTSensor.handle ();

}