#include "EnigmaIOTSensor.h"
#include "espnow_hal.h"
#define BLUE_LED 2
uint8_t gateway[6] = { 0x5E, 0xCF, 0x7F, 0x80, 0x34, 0x75 };

#define SLEEP_TIME 10000000

void connectEventHandler () {
    Serial.println ("Connected");
}

void disconnectEventHandler () {
    Serial.println ("Disconnected");
}

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    EnigmaIOTSensor.setLed (BLUE_LED);
    EnigmaIOTSensor.onConnected (connectEventHandler);
    EnigmaIOTSensor.onDisconnected (disconnectEventHandler);
    EnigmaIOTSensor.begin (&Espnow_hal, gateway);

    char *message = "Hello World!!!";

    Serial.printf ("Trying to send: %s\n", message);
    EnigmaIOTSensor.sendData ((uint8_t *)message, strlen (message));

    EnigmaIOTSensor.sleep (SLEEP_TIME);
}

void loop () {

    EnigmaIOTSensor.handle ();

}