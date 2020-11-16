// BasicController.h

#ifndef _DS18B20CONTROLLER_h
#define _DS18B20CONTROLLER_h

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
#define CONTROLLER_CLASS_NAME ds18b20Controller
static const char* CONTROLLER_NAME = "DS18B20 controller";

// --------------------------------------------------
// You may define data structures and constants here
// --------------------------------------------------
#include <DallasTemperature.h>


class CONTROLLER_CLASS_NAME : EnigmaIOTjsonController {
protected:
	// --------------------------------------------------
	// add all parameters that your project needs here
	// --------------------------------------------------
	OneWire* oneWire;
	DallasTemperature* sensors;
	DeviceAddress insideThermometer;

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

	bool sendTemperature (float temp);

	// ------------------------------------------------------------
	// You may add additional method definitions that you need here
	// ------------------------------------------------------------
};

#endif

