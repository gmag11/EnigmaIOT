#include "SecureSensorGateway.h"
#include "espnow_hal.h"

#define BLUE_LED 2
#define RED_LED 16

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    SecureSensorGateway.setRxLed (BLUE_LED);
    SecureSensorGateway.setTxLed (RED_LED);
    SecureSensorGateway.begin (&Espnow_hal);

}

void loop () {
    SecureSensorGateway.handle ();
}