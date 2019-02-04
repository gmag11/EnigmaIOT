// 
// 
// 

#include "helperFunctions.h"

#ifdef DEBUG_ESP_PORT
void printHexBuffer (byte *buffer, uint16_t len) {
    for (int i = 0; i < len; i++) {
        Serial.printf ("%02X ", buffer[i]);
    }
    Serial.println ();
}
#endif

void initWiFi () {
	WiFi.persistent (false);
	WiFi.mode (WIFI_AP);
	WiFi.softAP ("ESPNOW", nullptr, 3);
	WiFi.softAPdisconnect (false);

    DEBUG_MSG ("MAC address of this node is %s\n", WiFi.softAPmacAddress ().c_str());
}
