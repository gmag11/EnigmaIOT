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

//#define DEBUG_ESP_PORT Serial

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf ("%u %s ",millis(),__FUNCTION__); DEBUG_ESP_PORT.printf( __VA_ARGS__ )
void printHexBuffer (byte *buffer, uint16_t len);
#else
#define DEBUG_MSG(...)
#define printHexBuffer(...)
#endif



void initWiFi ();

#endif

