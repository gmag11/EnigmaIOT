#include "EspNowSensor.h"
#include "espnow_hal.h"
#define BLUE_LED 2

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    EspNowSensor.setLed (BLUE_LED);
    EspNowSensor.begin (&Espnow_hal);
}

void loop () {

    static time_t lastMessage;
    char *message = "Hello World!!!";

    EspNowSensor.handle ();

    if (millis () - lastMessage > 5000) {
        lastMessage = millis ();
        DEBUG_INFO ("Trying to send");
        EspNowSensor.sendData ((uint8_t *)message, strlen (message));
    }
}