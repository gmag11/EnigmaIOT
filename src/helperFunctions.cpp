/**
  * @file helperFunctions.cpp
  * @version 0.9.5
  * @date 30/10/2020
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

	if (buffer) {
		for (int i = 0; i < len; i++) {
			if (charIndex < MAX_STR_LEN - 2) {
				charIndex += sprintf (tempStr + charIndex, "%02X ", buffer[i]);
			}
		}
	}
	return tempStr;
}

void initWiFi (uint8_t channel, const char* networkName, const char* networkKey, uint8_t role) {
	DEBUG_DBG ("initWifi");
	if (role == 0) { // Node
		WiFi.mode (WIFI_STA);
#ifdef ESP32
		esp_err_t err_ok;
		if ((err_ok = esp_wifi_set_promiscuous (true))) {
			DEBUG_ERROR ("Error setting promiscuous mode: %s", esp_err_to_name (err_ok));
		}
		if ((err_ok = esp_wifi_set_channel (channel, WIFI_SECOND_CHAN_NONE))) {
			DEBUG_ERROR ("Error setting wifi channel: %s", esp_err_to_name (err_ok));
		}
		if ((err_ok = esp_wifi_set_promiscuous (false))) {
			DEBUG_ERROR ("Error setting promiscuous mode off: %s", esp_err_to_name (err_ok));
		}
#endif
		WiFi.disconnect ();
#ifdef ESP8266
		wifi_set_channel (channel);
#endif
		DEBUG_DBG ("Mode set to STA. Channel %u", channel);
	} else { // Gateway
		WiFi.mode (WIFI_AP);
		WiFi.softAP (networkName, networkKey, channel);
		DEBUG_DBG ("Mode set to AP in channel %u", channel);
	}

	DEBUG_INFO ("AP MAC address of this device is %s", WiFi.softAPmacAddress ().c_str ());
	DEBUG_INFO ("STA MAC address of this device is %s", WiFi.macAddress ().c_str ());

}

uint32_t calculateCRC32 (const uint8_t* data, size_t length) {
	uint32_t crc = 0xffffffff;
	while (length--) {
		uint8_t c = *data++;
		for (uint32_t i = 0x80; i > 0; i >>= 1) {
			bool bit = crc & 0x80000000;
			if (c & i) {
				bit = !bit;
			}
			crc <<= 1;
			if (bit) {
				crc ^= 0x04c11db7;
			}
		}
	}
	return crc;
}

#undef MACSTR
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

char* mac2str (const uint8_t* mac, char* buffer) {
	if (mac && buffer) {
		//DEBUG_DBG ("mac2str %02x:%02x:%02x:%02x:%02x:%02x",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		snprintf (buffer, ENIGMAIOT_ADDR_LEN * 3, MACSTR, MAC2STR (mac));
		//DEBUG_DBG ("Address: %s", buffer);
		return buffer;
	}
	return NULL;
}

uint8_t* str2mac (const char* macAddrString, uint8_t* macBytes) {
	const char cSep = ':';

	if (!macBytes) {
		return NULL;
	}

	for (int i = 0; i < 6; ++i) {
		unsigned int iNumber = 0;
		char ch;

		//Convert letter into lower case.
		ch = tolower (*macAddrString++);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			return NULL;
		}

		//Convert into number. 
		//  a. If character is digit then ch - '0'
		//	b. else (ch - 'a' + 10) it is done 
		//	      because addition of 10 takes correct value.
		iNumber = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
		ch = tolower (*macAddrString);

		if ((i < 5 && ch != cSep) ||
			(i == 5 && ch != '\0' && !isspace (ch))) {
			++macAddrString;

			if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
				return NULL;
			}

			iNumber <<= 4;
			iNumber += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
			ch = *macAddrString;

			if (i < 5 && ch != cSep) {
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

#ifdef ESP8266
const char* IRAM_ATTR extractFileName (const char* path) {
	size_t i = 0;
	size_t pos = 0;
	char* p = (char*)path;
	while (*p) {
		i++;
		if (*p == '/' || *p == '\\') {
			pos = i;
		}
		p++;
	}
	return path + pos;
}
#endif

bool isNumber (const char* input) {
	unsigned int index = 0;
	size_t len = strlen (input);

	if (!len) {
		return false;
	}

	while (index < len) {
		if (!isDigit (input[index])) {
			return false;
		}
		index++;
	}
	return true;
}

bool isNumber (const char* input, size_t len) {
	unsigned int index = 0;

	if (!len) {
		return false;
	}

	while (input[index] != '\0' && index < len) {
		if (!isDigit (input[index])) {
			return false;
		}
		index++;
	}
	return true;
}

bool isNumber (String input) {
	unsigned int index = 0;
	size_t len = input.length ();

	if (!len) {
		return false;
	}

	while (index < len) {
		if (!isDigit (input[index])) {
			return false;
		}
		index++;
	}
	return true;
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
