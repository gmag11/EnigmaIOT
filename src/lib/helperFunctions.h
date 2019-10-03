/**
  * @file helperFunctions.h
  * @version 0.5.0
  * @date 03/10/2019
  * @author German Martin
  * @brief Auxiliary function definition
  */

#ifndef _HELPERFUNCTIONS_h
#define _HELPERFUNCTIONS_h


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "EnigmaIoTconfig.h"
#include "debug.h"

  /**
	* @brief Debug helper function that generates a string that represent a buffer hexadecimal values
	* @param buffer Pointer to the buffer
	* @param len Buffer length in number of bytes
	* @return Returns a pointer to the generated string.
	*
	* String has to be used inmediatelly. At least before calling `printHexBuffer()` again as it uses a static buffer to hold string.
	* It will be overwritten on next call.
	*/
char* printHexBuffer (const uint8_t* buffer, uint16_t len);

/**
  * @brief Debug helper function that generates a string that represent a MAC address
  * @param mac Pointer to the MAC address
  * @param buffer Buffer that will store resulting address. It must be 18 bytes long at least
  * @return Returns a pointer to input buffer after writting MAC address on human readable format
  */
const char* mac2str (const uint8_t* mac, const char* buffer);

uint8_t* str2mac (const char* mac, uint8_t* values);

/**
  * @brief Initalizes WiFi interfaces on ESP8266 or ESP32
  * @param channel WiFi channel for interface initialization
  * @param role 0 for node, 1 for gateway
  */
void initWiFi (uint8_t channel = 3, uint8_t role = 0, String networkName = "EnigmaIOT");

#endif

