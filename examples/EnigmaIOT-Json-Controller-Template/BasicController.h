// BasicController.h

#ifndef _BASICCONTROLLER_h
#define _BASICCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//#define DEBUG_SERIAL

#include <EnigmaIOTjsonController.h>
#define CONTROLLER_CLASS_NAME BasicController
static const char* CONTROLLER_NAME = "EnigmaIOT controller template";

// --------------------------------------------------
// You may define data structures and constants here
// --------------------------------------------------

class CONTROLLER_CLASS_NAME : EnigmaIOTjsonController {
protected:
	// --------------------------------------------------
	// add all parameters that your project needs here
	// --------------------------------------------------

public:
    /**
     * @brief Initializes controller structures
     * @param node Pointer to EnigmaIOT gateway instance
     * @param data Parameter data for controller
     */
	void setup (EnigmaIOTNodeClass* node, void* data = NULL);

    /**
     * @brief Processes received GET or SET commands
     * @param address Origin MAC address
     * @param buffer Command payload
     * @param length Payload length in bytes
     * @param command Command type. nodeMessageType_t::DOWNSTREAM_DATA_GET or nodeMessageType_t::DOWNSTREAM_DATA_SET
     * @param payloadEncoding Payload encoding. MSG_PACK is recommended
     */
	bool processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding);

    /**
     * @brief Executes repetitive tasks on controller
     */
	void loop ();

	~CONTROLLER_CLASS_NAME ();

	/**
	 * @brief Called when wifi manager starts config portal
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

    /**
     * @brief Executed as soon as node is registered on EnigmaIOT network
     */
    void connectInform () {
		sendStartAnouncement ();
	}

protected:
	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	bool saveConfig ();

    /**
     * @brief Send response to commands to gateway
     * @param command Refered command
     * @param result `true` if command was correctly executed, `false` otherwise
     */
	bool sendCommandResp (const char* command, bool result);

    /**
     * @brief Sends a notification message including configurable controller name and protocol version
     */
    bool sendStartAnouncement () {
        // You can send a 'hello' message when your node starts. Useful to detect unexpected reboot
        const size_t capacity = JSON_OBJECT_SIZE (10);
        DynamicJsonDocument json (capacity);
        json["status"] = "start";
        json["device"] = CONTROLLER_NAME;
        char version_buf[10];
        snprintf (version_buf, 10, "%d.%d.%d",
                  ENIGMAIOT_PROT_VERS[0], ENIGMAIOT_PROT_VERS[1], ENIGMAIOT_PROT_VERS[2]);
        json["version"] = String (version_buf);

        return sendJson (json);
    }

	// ------------------------------------------------------------
	// You may add additional method definitions that you need here
	// ------------------------------------------------------------
};

#endif

