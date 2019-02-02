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

void initWiFi () {
	WiFi.persistent (false);
	WiFi.mode (WIFI_AP);
	WiFi.softAP ("ESPNOW", nullptr, 3);
	WiFi.softAPdisconnect (false);

	Serial.print ("MAC address of this node is ");
	Serial.println (WiFi.softAPmacAddress ());
}
