// BasicController.h

#ifndef _BUTTONCONTROLLER_h
#define _BUTTONCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#ifdef ESP32
#include <SPIFFS.h>
#endif

#include <EnigmaIOTjsonController.h>

// --------------------------------------------------
// You may define data structures and constants here
// --------------------------------------------------
#define BUTTON_PIN 4

#define CONTROLLER_CLASS_NAME ButtonController
static const char* CONTROLLER_NAME = "Button controller";

class CONTROLLER_CLASS_NAME : EnigmaIOTjsonController {
protected:
	// --------------------------------------------------
	// add all parameters that your project needs here
	// --------------------------------------------------
	bool pushTriggered = false;
	bool pushReleased = true;

public:
	void setup (EnigmaIOTNodeClass* node, void* data = NULL);

	bool processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding);

	void loop ();

	~CONTROLLER_CLASS_NAME ();

	/**
	 * @brief Called when wifi manager starts config portal
	 * @param node Pointer to EnigmaIOT gateway instance
	 */
	void configManagerStart ();

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

	void connectInform () {
		sendStartAnouncement ();
	}

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

