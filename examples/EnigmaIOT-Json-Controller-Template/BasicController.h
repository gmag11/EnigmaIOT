// BasicController.h

#ifndef _BASICCONTROLLER_h
#define _BASICCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

//#define DEBUG_SERIAL

#ifdef ESP32
#include <SPIFFS.h>
#endif

#include <EnigmaIOTjsonController.h>
#define CONTROLLER_CLASS_NAME BasicController

// --------------------------------------------------
// You may define data structures and constants here
// --------------------------------------------------

class CONTROLLER_CLASS_NAME : EnigmaIOTjsonController {
protected:
	// --------------------------------------------------
	// add all parameters that your project needs here
	// --------------------------------------------------

public:
	void setup (void* data = NULL);
	
	bool processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding);
	
	void loop ();
	
	~CONTROLLER_CLASS_NAME ();

	/**
	 * @brief Called when wifi manager starts config portal
	 * @param node Pointer to EnigmaIOT gateway instance
	 */
	void configManagerStart (EnigmaIOTNodeClass* node);

	/**
	 * @brief Called when wifi manager exits config portal
	 * @param status `true` if configuration was successful
	 */
	void configManagerExit (bool status);

	/**
	 * @brief Loads output module configuration
	 * @return Returns `true` if load was successful. `false` otherwise
	 */
	bool loadConfig ();

protected:
	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	bool saveConfig ();
	
	bool sendCommandResp (const char* command, bool result);
	
	bool sendStartAnouncement ();

	// ------------------------------------------------------------
	// You may add additional method definitions that you need here
	// ------------------------------------------------------------
};

#endif

