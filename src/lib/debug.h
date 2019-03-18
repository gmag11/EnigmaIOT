/**
  * @brief Auxiliary functions for debugging over Serial
  *
  * Format used on debug functions is the same as `printf()`. Check source code for usage examples
  * Debug calls will be enabled or disabled automatically before compiling according defined `DEBUG_LEVEL`.
  *
  * If `DEBUG_ESP_PORT` is not defined library will give no debug output at all
  *
  * @file debug.h
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  */

#ifndef _DEBUG_h
#define _DEBUG_h

//#define DEBUG_ESP_PORT Serial

#define NO_DEBUG	0 ///< @brief Debug level that will give no debug output
#define ERROR	1 ///< @brief Debug level that will give error messages
#define WARN	2 ///< @brief Debug level that will give error and warning messages
#define INFO	3 ///< @brief Debug level that will give error, warning and info messages
#define VERBOSE	4 ///< @brief Debug level that will give all defined messages

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



#endif

