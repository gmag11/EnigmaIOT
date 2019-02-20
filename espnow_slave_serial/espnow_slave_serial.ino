#include "SecureSensorGateway.h"
#include "espnow_hal.h"

#define BLUE_LED 2
#define RED_LED 16

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

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    SecureSensorGateway.setRxLed (BLUE_LED);
    SecureSensorGateway.setTxLed (RED_LED);
    SecureSensorGateway.begin (&Espnow_hal);
    SecureSensorGateway.onDataRx (processRxData);
}

void loop () {
    SecureSensorGateway.handle ();
}