// helperFunctions.h

#ifndef _HELPERFUNCTIONS_h
#define _HELPERFUNCTIONS_h


#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "config.h"
#include "debug.h"

char* printHexBuffer (const uint8_t *buffer, uint16_t len);

const char *mac2str (const uint8_t *mac, const char *buffer);

void initWiFi ();

#endif

