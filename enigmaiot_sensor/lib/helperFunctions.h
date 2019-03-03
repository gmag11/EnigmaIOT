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

#define NO_DEBUG	0
#define ERROR	1
#define WARN	2
#define INFO	3
#define VERBOSE	4

#define DEBUG_LINE_PREFIX() DEBUG_ESP_PORT.printf ("[%u] %u free (%s:%d) ",millis(),ESP.getFreeHeap(),__FUNCTION__,__LINE__);

#ifdef DEBUG_ESP_PORT
    #define DEBUG_LEVEL VERBOSE

	#if DEBUG_LEVEL >= VERBOSE
		#define DEBUG_VERBOSE(...) DEBUG_ESP_PORT.print("V "); DEBUG_LINE_PREFIX(); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_VERBOSE(...)
	#endif

	#if DEBUG_LEVEL >= INFO
		#define DEBUG_INFO(...) DEBUG_ESP_PORT.print("I "); DEBUG_LINE_PREFIX(); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_INFO(...)
	#endif

	#if DEBUG_LEVEL >= WARN
		#define DEBUG_WARN(...) DEBUG_ESP_PORT.print("W "); DEBUG_LINE_PREFIX(); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_WARN(...)
	#endif
	
	#if DEBUG_LEVEL >= ERROR
		#define DEBUG_ERROR(...) DEBUG_ESP_PORT.print("E "); DEBUG_LINE_PREFIX(); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_ERROR(...)
	#endif
#else
#define DEBUG_VERBOSE(...)
#define DEBUG_INFO(...)
#define DEBUG_WARN(...)
#define DEBUG_ERROR(...)
#endif

char* printHexBuffer (const uint8_t *buffer, uint16_t len);

bool mac2str (const uint8_t *mac, const char *buffer);

void initWiFi ();

#endif

