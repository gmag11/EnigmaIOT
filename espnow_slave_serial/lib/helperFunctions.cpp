// 
// 
// 

#include "helperFunctions.h"

#ifdef DEBUG_ESP_PORT
char *printHexBuffer (const uint8_t *buffer, uint16_t len) {
	static char tempStr[150];
	int charIndex = 0;
	//char *chrPtr = tempStr;

    for (int i = 0; i < len; i++) {
		charIndex += sprintf (tempStr + charIndex, "%02X ", buffer[i]);
		//Serial.printf ("%s\n", tempStr);
		//chrPtr += charIndex;
        //Serial.printf ("%02X ", buffer[i]);
    }
	//sprintf (tempStr, "\n");
    //Serial.println ();
	return tempStr;
}
#endif

void initWiFi () {
    //WiFi.preinitWiFiOff ();
	//WiFi.persistent (false);
	WiFi.mode (WIFI_AP);
	WiFi.softAP ("ESPNOW", nullptr, 3);
	WiFi.softAPdisconnect (false);

    DEBUG_INFO ("AP MAC address of this device is %s", WiFi.softAPmacAddress ().c_str());
    DEBUG_INFO ("STA MAC address of this device is %s", WiFi.macAddress ().c_str ());

}

bool mac2str (const uint8_t *mac, const char *buffer) {
    if (mac && buffer) {
        sprintf (const_cast<char *>(buffer), MACSTR, MAC2STR (mac));
    }
}
