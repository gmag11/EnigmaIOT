// helperFunctions.h

#ifndef _HELPERFUNCTIONS_h
#define _HELPERFUNCTIONS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void printHexBuffer (byte *buffer, uint16_t len);

#endif

