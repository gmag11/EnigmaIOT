// 
// 
// 

#include "helperFunctions.h"

#ifdef DEBUG_ESP_PORT
char *printHexBuffer (const uint8_t *buffer, uint16_t len) {
	static char tempStr[200];
	int charIndex = 0;

    for (int i = 0; i < len; i++) {
		charIndex += sprintf (tempStr + charIndex, "%02X ", buffer[i]);
    }
	return tempStr;
}
#endif

void initWiFi () {
	//WiFi.persistent (false);
	WiFi.mode (WIFI_AP);
	WiFi.softAP ("ESPNOW", nullptr, 3);
	WiFi.softAPdisconnect (false);

    DEBUG_INFO ("AP MAC address of this device is %s", WiFi.softAPmacAddress ().c_str());
    DEBUG_INFO ("STA MAC address of this device is %s", WiFi.macAddress ().c_str ());

}

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

bool mac2str (const uint8_t *mac, const char *buffer) {
    if (mac && buffer) {
        sprintf (const_cast<char *>(buffer), MACSTR, MAC2STR (mac));
    }
}
