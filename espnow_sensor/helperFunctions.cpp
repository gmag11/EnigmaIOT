// 
// 
// 

#include "helperFunctions.h"

void printHexBuffer (byte *buffer, uint16_t len) {
    for (int i = 0; i < len; i++) {
        Serial.printf ("%02X ", buffer[i]);
    }
    Serial.println ();
}
