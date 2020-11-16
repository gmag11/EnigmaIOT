// 
// 
// 

#include <functional>
#include "ButtonController.h"

using namespace std;
using namespace placeholders;

constexpr auto CONFIG_FILE = "/customconf.json"; ///< @brief Custom configuration file name

// -----------------------------------------
// You may add some global variables you need here,
// like serial port instances, I2C, etc
// -----------------------------------------


bool CONTROLLER_CLASS_NAME::processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) {
	// Process incoming messages here
	// They are normally encoded as MsgPack so you can convert them to JSON very easily
	return true;
}


bool CONTROLLER_CLASS_NAME::sendCommandResp (const char* command, bool result) {
	// Respond to command with a result: true if successful, false if failed 
	return true;
}

bool CONTROLLER_CLASS_NAME::sendStartAnouncement () {
	// You can send a 'hello' message when your node starts. Useful to detect unexpected reboot
	const size_t capacity = JSON_OBJECT_SIZE (2);
	DynamicJsonDocument json (capacity);
	json["status"] = "start";
	json["device"] = CONTROLLER_NAME;

	return sendJson (json);
}

void CONTROLLER_CLASS_NAME::setup (EnigmaIOTNodeClass *node, void *data) {
	enigmaIotNode = node;
	// You do node setup here. Use it as it was the normal setup() Arduino function
	pinMode (BUTTON_PIN, INPUT_PULLUP);

	// Send a 'hello' message when initalizing is finished
	sendStartAnouncement ();

	DEBUG_DBG ("Finish begin");

	// If your node should sleep after sending data do all remaining tasks here
}

void CONTROLLER_CLASS_NAME::loop () {

	// If your node stays allways awake do your periodic task here

	if (pushReleased) { // Enter this only if button were not pushed in the last loop
		if (!digitalRead (BUTTON_PIN)) {
			delay (50); // debounce button push
			if (!digitalRead (BUTTON_PIN)) {
				DEBUG_WARN ("Button triggered!");
				pushTriggered = true; // Button is pushed
				pushReleased = false; // Mark button as not released
			}
		}
	}

	if (pushTriggered) { // If button was pushed
		pushTriggered = false; // Disable push trigger
		const size_t capacity = JSON_OBJECT_SIZE (2);
		DynamicJsonDocument json (capacity);
		json["button"] = BUTTON_PIN;
		json["push"] = 1;
		if (sendJson (json)) {
			DEBUG_WARN ("Push triggered sent");
		} else {
			DEBUG_ERROR ("Push send error");
		}
	}

	if (!pushReleased) {
		if (digitalRead (BUTTON_PIN)) { // If button is released
			DEBUG_WARN ("Button released");
			pushReleased = true;
			const size_t capacity = JSON_OBJECT_SIZE (2);
			DynamicJsonDocument json (capacity);
			json["button"] = BUTTON_PIN;
			json["push"] = 0;
			if (sendJson (json)) {
				DEBUG_WARN ("Push released sent");
			} else {
				DEBUG_ERROR ("Push send error");
			}
		}
	}
}

CONTROLLER_CLASS_NAME::~CONTROLLER_CLASS_NAME () {
	// If your class uses dynamic data free it up here
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