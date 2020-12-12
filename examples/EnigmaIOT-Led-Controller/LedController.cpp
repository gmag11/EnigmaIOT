// 
// 
// 

#include <functional>
#include "LedController.h"

using namespace std;
using namespace placeholders;

constexpr auto CONFIG_FILE = "/customconf.json"; ///< @brief Custom configuration file name

// -----------------------------------------
// You may add some global variables you need here,
// like serial port instances, I2C, etc
// -----------------------------------------

const char* ledKey = "led";
const char* commandKey = "cmd";


bool CONTROLLER_CLASS_NAME::processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) {
	// Process incoming messages here
	// They are normally encoded as MsgPack so you can confert them to JSON very easily

	// Check command type
	if (command != nodeMessageType_t::DOWNSTREAM_DATA_GET && command != nodeMessageType_t::DOWNSTREAM_DATA_SET) {
		DEBUG_WARN ("Wrong message type");
		return false;
	}
	// Check payload encoding
	if (payloadEncoding != MSG_PACK) {
		DEBUG_WARN ("Wrong payload encoding");
		return false;
	}

	// Decode payload
	DynamicJsonDocument doc (256);
	uint8_t tempBuffer[MAX_MESSAGE_LENGTH];

	memcpy (tempBuffer, buffer, length);
	DeserializationError error = deserializeMsgPack (doc, tempBuffer, length);
	// Check decoding
	if (error != DeserializationError::Ok) {
		DEBUG_WARN ("Error decoding command: %s", error.c_str ());
		return false;
	}

	DEBUG_WARN ("Command: %d = %s", command, command == nodeMessageType_t::DOWNSTREAM_DATA_GET ? "GET" : "SET");

	// Dump debug data
	size_t strLen = measureJson (doc) + 1;
	char* strBuffer = (char*)malloc (strLen);
	serializeJson (doc, strBuffer, strLen);
	DEBUG_WARN ("Data: %s", strBuffer);
	free (strBuffer);

	// Check cmd field on JSON data
	if (!doc.containsKey (commandKey)) {
		DEBUG_WARN ("Wrong format");
		return false;
	}

	// Get state
	if (command == nodeMessageType_t::DOWNSTREAM_DATA_GET) {
		if (!strcmp (doc[commandKey], ledKey)) {
			DEBUG_WARN ("Request LED status. LED = %s", led == LED_ON ? "ON" : "OFF");
			if (!sendLedStatus ()) {
				DEBUG_WARN ("Error sending LED status");
				return false;
			}
		}
	}

	// Set state
	if (command == nodeMessageType_t::DOWNSTREAM_DATA_SET) {
		if (!strcmp (doc[commandKey], ledKey)) {
			if (doc.containsKey (ledKey)) {
				if (doc[ledKey].as<int> () == 1) {
					led = LED_ON;
				} else if (doc[ledKey].as<int> () == 0) {
					led = LED_OFF;
				} else {
					DEBUG_WARN ("Wrong LED value: %d", doc[ledKey].as<int> ());
					return false;
				}
				DEBUG_WARN ("Set LED status to %s", led == LED_ON ? "ON" : "OFF");
			} else {
				DEBUG_WARN ("Wrong format");
				return false;
			}

			// Confirm command execution with send state
			if (!sendLedStatus ()) {
				DEBUG_WARN ("Error sending LED status");
				return false;
			}
		}
	}


	return true;
}

bool CONTROLLER_CLASS_NAME::sendLedStatus () {
	const size_t capacity = JSON_OBJECT_SIZE (2);
	DynamicJsonDocument json (capacity);

	json[commandKey] = ledKey;
	json[ledKey] = led;

	return sendJson (json);
}


bool CONTROLLER_CLASS_NAME::sendCommandResp (const char* command, bool result) {
	// Respond to command with a result: true if successful, false if failed 
	return true;
}

void CONTROLLER_CLASS_NAME::setup (EnigmaIOTNodeClass* node, void* data) {
	enigmaIotNode = node;

	// You do node setup here. Use it as it was the normal setup() Arduino function
	pinMode (LED_PIN, OUTPUT);
	digitalWrite (LED_PIN, LED_ON);

	// Send a 'hello' message when initalizing is finished
	sendStartAnouncement ();

	DEBUG_DBG ("Finish begin");

	// If your node should sleep after sending data do all remaining tasks here
}

void CONTROLLER_CLASS_NAME::loop () {

	// If your node stays allways awake do your periodic task here
	digitalWrite (LED_PIN, led);
}

CONTROLLER_CLASS_NAME::~CONTROLLER_CLASS_NAME () {
	// It your class uses dynamic data free it up here
	// This is normally not needed but it is a good practice
}

void CONTROLLER_CLASS_NAME::configManagerStart () {
	DEBUG_INFO ("==== CCost Controller Configuration start ====");
	// If you need to add custom configuration parameters do it here
}

void CONTROLLER_CLASS_NAME::configManagerExit (bool status) {
	DEBUG_INFO ("==== CCost Controller Configuration result ====");
	// You can read configuration paramenter values here
}

bool CONTROLLER_CLASS_NAME::loadConfig () {
	// If you need to read custom configuration data do it here
	return true;
}

bool CONTROLLER_CLASS_NAME::saveConfig () {
	// If you need to save custom configuration data do it here
	return true;
}
