/**
  * @file helperFunctions.cpp
  * @version 0.6.0
  * @date 17/11/2019
  * @author German Martin
  * @brief Auxiliary function definition
  */

#include "helperFunctions.h"
#ifdef ESP32
#include <esp_wifi.h>
#endif

#define MAX_STR_LEN 1000 ///< @brief Key length used by selected crypto algorythm

char* printHexBuffer (const uint8_t* buffer, uint16_t len) {
	static char tempStr[MAX_STR_LEN];
	int charIndex = 0;

	memset (tempStr, 0, MAX_STR_LEN);

	for (int i = 0; i < len; i++) {
		if (charIndex < MAX_STR_LEN - 2) {
			charIndex += sprintf (tempStr + charIndex, "%02X ", buffer[i]);
		}
	}
	return tempStr;
}

void initWiFi (uint8_t channel, uint8_t role, String networkName) {
	DEBUG_DBG ("initWifi");
	//DEBUG_DBG ("AP started");
	if (role == 0) { // Node
		//WiFi.softAP ("ESPNOW", "qpwoeirufjdhbfdjd", channel, true);
		//DEBUG_DBG ("Mode set to AP in channel %u", channel);
		WiFi.mode (WIFI_STA);
		WiFi.disconnect ();
		wifi_set_channel (channel);
		DEBUG_DBG ("Mode set to STA. Channel %u", channel);
	} else { // Gateway
		WiFi.mode (WIFI_AP);
		WiFi.softAP (networkName.c_str(), "2599657852368549566551", channel); // TODO: password should be true random
		DEBUG_DBG ("Mode set to AP in channel %u", channel);
	}

	DEBUG_INFO ("AP MAC address of this device is %s", WiFi.softAPmacAddress ().c_str ());
	DEBUG_INFO ("STA MAC address of this device is %s", WiFi.macAddress ().c_str ());

}

#undef MACSTR
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

char* mac2str (const uint8_t* mac, char* buffer) {
	if (mac && buffer) {
		//DEBUG_DBG ("mac2str %02x:%02x:%02x:%02x:%02x:%02x",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		snprintf (buffer, 18, MACSTR, MAC2STR (mac));
		//DEBUG_DBG ("Address: %s", buffer);
		return buffer;
	}
	return NULL;
}

uint8_t* str2mac (const char* macAddrString, uint8_t* macBytes)
{
	const char cSep = ':';

	for (int i = 0; i < 6; ++i)	{
		unsigned int iNumber = 0;
		char ch;

		//Convert letter into lower case.
		ch = tolower (*macAddrString++);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
		{
			return NULL;
		}

		//Convert into number. 
		//  a. If character is digit then ch - '0'
		//	b. else (ch - 'a' + 10) it is done 
		//	      because addition of 10 takes correct value.
		iNumber = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
		ch = tolower (*macAddrString);

		if ((i < 5 && ch != cSep) ||
			(i == 5 && ch != '\0' && !isspace (ch)))
		{
			++macAddrString;

			if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
			{
				return NULL;
			}

			iNumber <<= 4;
			iNumber += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
			ch = *macAddrString;

			if (i < 5 && ch != cSep)
			{
				return NULL;
			}
		}
		/* Store result.  */
		macBytes[i] = (unsigned char)iNumber;
		/* Skip cSep.  */
		++macAddrString;
	}
	return macBytes;
}

//int str2mac (const char* mac, uint8_t* values) {
//	int error = std::sscanf (mac, "%02x:%02x:%02x:%02x:%02x:%02x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
//	Serial.printf ("Error: %d", error);
//	if (error == 6) {
//		for (int i = 0; i < 6; i++) {
//			Serial.println (values[i]);
//		}
//		return 1;
//	}
//	else {
//		return 0;
//	}
//}
