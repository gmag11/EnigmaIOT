/**
  * @file EnigmaIOTjsonController.h
  * @version 0.9.3
  * @date 14/07/2020
  * @author German Martin
  * @brief Prototype for JSON/MSGPACK based controller node
  */

#ifndef _ENIGMAIOTJSONCONTROLLER_h
#define _ENIGMAIOTJSONCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <EnigmaIOTNode.h>
#include <ArduinoJson.h>

#if defined ESP8266 || defined ESP32
#include <functional>
typedef std::function<bool (const uint8_t* data, size_t len, nodePayloadEncoding_t payloadEncoding)> sendData_cb; /**< Data send callback definition */
#else
#error This code only supports ESP8266 or ESP32 platforms
#endif

class EnigmaIOTjsonController {
protected:
	sendData_cb sendData;
	EnigmaIOTNodeClass* enigmaIotNode;

public:
   /**
	 * @brief Initialize data structures
	 * @param config Pointer to configuration structure. If it is `NULL` then it tries to load configuration from flash
	 */
	virtual void setup (void* config = NULL) = 0;

	/**
	 * @brief This should be called periodically for module handling
	 */
	virtual void loop () = 0;

	/**
	 * @brief Called to process a downlink command
	 * @param mac Address of sender
	 * @param buffer Message bytes
	 * @param length Message length
	 * @param command Type of command. nodeMessageType_t
	 * @param payloadEncoding Payload encoding of type nodePayloadEncoding_t
	 * @return `true` on success
	 */
	virtual bool processRxCommand (
		const uint8_t* mac, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) = 0;

	/**
	 * @brief Register send data callback to run when module needs to send a message
	 * @param cb Callback with sendData_cb format
	 */
	void sendDataCallback (sendData_cb cb) {
		sendData = cb;
	}

	/**
	 * @brief Called when wifi manager starts config portal
	 * @param enigmaIotNode Pointer to EnigmaIOT node instance
	 */
	virtual void configManagerStart (EnigmaIOTNodeClass* node) = 0;

	/**
	 * @brief Called when wifi manager exits config portal
	 * @param status `true` if configuration was successful
	 */
	virtual void configManagerExit (bool status) = 0;

	/**
	 * @brief Loads output module configuration
	 * @return Returns `true` if load was successful. `false` otherwise
	 */
	virtual bool loadConfig () = 0;

protected:

	/**
	  * @brief Sends command processing response acknowledge
	  * @param command Command name
	  * @param result Command execution success
	  * @return Returns `true` if message sending was successful. `false` otherwise
	  */
	virtual bool sendCommandResp (const char* command, bool result) = 0;

	/**
	  * @brief Send a message to notify node has started running
	  * @return Returns `true` if message sending was successful. `false` otherwise
	  */
	virtual bool sendStartAnouncement () = 0;

	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	virtual bool saveConfig () = 0;

	/**
	  * @brief Sends a JSON encoded message to lower layer
	  * @return Returns `true` if message sending was successful. `false` otherwise
	  */
	bool sendJson (DynamicJsonDocument& json) {
		int len = measureMsgPack (json) + 1;
		uint8_t* buffer = (uint8_t*)malloc (len);
		len = serializeMsgPack (json, (char*)buffer, len);

		size_t strLen = measureJson (json) + 1;
		char* strBuffer = (char*)calloc (sizeof (uint8_t), strLen);

		/*Serial.printf ("Trying to send: %s\n", printHexBuffer (
			buffer, len));*/
		serializeJson (json, strBuffer, strLen);
		DEBUG_INFO ("Trying to send: %s", strBuffer);
		bool result = false;
		if (sendData)
			result = sendData (buffer, len, MSG_PACK);
		if (!result) {
			DEBUG_WARN ("---- Error sending data");
		} else {
			DEBUG_INFO ("---- Data sent");
		}
		free (buffer);
		free (strBuffer);
		return result;
	}
};

#endif // _ENIGMAIOTJSONCONTROLLER_h

