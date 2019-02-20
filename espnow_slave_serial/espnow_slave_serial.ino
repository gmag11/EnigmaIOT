#include "SecureSensorGateway.h"
#include "espnow_hal.h"

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    SecureSensorGateway.begin (&Espnow_hal);

}

void loop () {
    SecureSensorGateway.handle ();
}