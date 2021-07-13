/**
  * @file helperFunctions.h
  * @version 0.9.8
  * @date 15/07/2021
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
#include "EnigmaIOTdebug.h"

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
  * @param extBuffer Buffer that will store resulting address. It must be 18 bytes long at least. If it is NULL a static buffer will be used
  * @return Returns a pointer to input buffer after writting MAC address on human readable format
  */
char* mac2str (const uint8_t* mac, char* extBuffer = NULL);

/**
  * @brief Debug helper function that creates MAC address byte array from text representation
  * @param mac Pointer to the MAC address string
  * @param values Buffer that will store byte array. It must be 6 bytes long at least
  * @return Returns a pointer to `values` input buffer after writting MAC address
  */
uint8_t* str2mac (const char* mac, uint8_t* values);

/**
  * @brief Initalizes WiFi interfaces on ESP8266 or ESP32
  * @param channel WiFi channel for interface initialization
  * @param role 0 for node, 1 for gateway
  * @param networkName Name that gateway AP will take
  * @param networkKey Network key. This is not required normally as this is provided using configuration web portal
  */
void initWiFi (uint8_t channel, const char* networkName, const char* networkKey = NULL, uint8_t role = 0);

/**
  * @brief Calculates CRC32 of a buffer
  * @param data Input buffer
  * @param length Input length
  * @return CRC32 value
  */
uint32_t calculateCRC32 (const uint8_t* data, size_t length);

/**
  * @brief Checks if input string is numeric
  * @param input String to check
  * @return `true` if string is a number
  */
bool isNumber (const char* input);

/**
  * @brief Checks if input string is numeric
  * @param input String to check
  * @param len String length
  * @return `true` if string is a number
  */
bool isNumber (const char* input, size_t len);

/**
  * @brief Checks if input string is numeric
  * @param input String to check
  * @return `true` if string is a number
  */
bool isNumber (String input);

#endif

