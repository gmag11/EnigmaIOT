// 
// 
// 

#include <functional>
#include "ds18b20Controller.h"

using namespace std;
using namespace placeholders;

constexpr auto CONFIG_FILE = "/customconf.json"; ///< @brief Custom configuration file name

// -----------------------------------------
// You may add some global variables you need here,
// like serial port instances, I2C, etc
// -----------------------------------------

#define ONE_WIRE_BUS 4


bool CONTROLLER_CLASS_NAME::processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) {
	// Process incoming messages here
	// They are normally encoded as MsgPack so you can confert them to JSON very easily
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

	return sendJson (json);
}

bool CONTROLLER_CLASS_NAME::sendTemperature (float temp) {
	const size_t capacity = JSON_OBJECT_SIZE (2);
	DynamicJsonDocument json (capacity);
	json["temp"] = temp;

	return sendJson (json);
}

void CONTROLLER_CLASS_NAME::setup (void* data) {

	// You do node setup here. Use it as it was the normal setup() Arduino function
	float tempC;

	oneWire = new OneWire (ONE_WIRE_BUS);
	sensors = new DallasTemperature (oneWire);
	sensors->begin ();
	sensors->setWaitForConversion (false);
	sensors->requestTemperatures ();
	time_t start = millis ();

	// Send a 'hello' message when initalizing is finished
	sendStartAnouncement ();

	while (!sensors->isConversionComplete ()) {
		delay (0);
	}
	DEBUG_WARN ("Conversion completed in %d ms", millis () - start);
	tempC = sensors->getTempCByIndex (0);

	sendTemperature (tempC);

	DEBUG_DBG ("Finish begin");

	// If your node should sleep after sending data do all remaining tasks here

}

void CONTROLLER_CLASS_NAME::loop () {

	// If your node stays allways awake do your periodic task here

	// You can send your data as JSON. This is a basic example

		//const size_t capacity = JSON_OBJECT_SIZE (4);
		//DynamicJsonDocument json (capacity);
		//json["sensor"] = data_description;
		//json["meas"] = measurement;

		//sendJson (json);

}

CONTROLLER_CLASS_NAME::~CONTROLLER_CLASS_NAME () {
	// It your class uses dynamic data free it up here
	// This is normally not needed but it is a good practice
}

void CONTROLLER_CLASS_NAME::configManagerStart (EnigmaIOTNodeClass* node) {
	enigmaIotNode = node;
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