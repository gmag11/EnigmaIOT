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

#define DEBUG_ESP_PORT Serial

#define NONE	0
#define ERROR	1
#define WARN	2
#define INFO	3
#define VERBOSE	4

#define DEGUG_LEVEL VERBOSE

#ifdef DEBUG_ESP_PORT
char* printHexBuffer (byte *buffer, uint16_t len);

	#if DEGUG_LEVEL >= VERBOSE
		#define DEBUG_VERBOSE(...) DEBUG_ESP_PORT.printf ("V [%u] (%s:%d) ",millis(),__FUNCTION__,__LINE__); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_VERBOSE(...)
	#endif

	#if DEGUG_LEVEL >= INFO
		#define DEBUG_INFO(...) DEBUG_ESP_PORT.printf ("I [%u] (%s:%d) ",millis(),__FUNCTION__,__LINE__); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_INFO(...)
	#endif

	#if DEGUG_LEVEL >= WARN
		#define DEBUG_WARN(...) DEBUG_ESP_PORT.printf ("W [%u] (%s:%d) ",millis(),__FUNCTION__,__LINE__); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_WARN(...)
	#endif
	
	#if DEGUG_LEVEL >= ERROR
		#define DEBUG_ERROR(...) DEBUG_ESP_PORT.printf ("E [%u] (%s:%d) ",millis(),__FUNCTION__,__LINE__); DEBUG_ESP_PORT.printf( __VA_ARGS__ ); DEBUG_ESP_PORT.println()
	#else
		#define DEBUG_ERROR(...)
	#endif
#else
#define printHexBuffer(...)
#define DEBUG_VERBOSE(...)
#define DEBUG_INFO(...)
#define DEBUG_WARN(...)
#define DEBUG_ERROR(...)
#endif



void initWiFi ();

#endif

