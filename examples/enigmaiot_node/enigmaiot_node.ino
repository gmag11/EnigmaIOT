/**
  * @file enigmaiot_node.ino
  * @version 0.5.0
  * @date 03/10/2019
  * @author German Martin
  * @brief Node based on EnigmaIoT over ESP-NOW
  *
  * Sensor reading code is mocked on this example. You can implement any other code you need for your specific need
  */

#include <Arduino.h>
#include <EnigmaIOTNode.h>
#include <espnow_hal.h>
#include <CayenneLPP.h>

#define BLUE_LED LED_BUILTIN
constexpr auto RESET_PIN = 13;
  //uint8_t gateway[6] = { 0x5E, 0xCF, 0x7F, 0x80, 0x34, 0x75 };
uint8_t gateway[6] = { 0xBE, 0xDD, 0xC2, 0x24, 0x14, 0x97 };

//#define SLEEP_TIME 20000000
ADC_MODE (ADC_VCC);

void connectEventHandler () {
	Serial.println ("Connected");
}

void disconnectEventHandler () {
	Serial.println ("Disconnected");
}

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length) {
	char macstr[18];
	mac2str (mac, macstr);
	Serial.println ();
	Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));
	for (int i = 0; i < length; i++) {
		Serial.print ((char)buffer[i]);
	}
	Serial.println ();
	Serial.println ();
}

void setup () {

	Serial.begin (115200); Serial.println (); Serial.println ();
	time_t start = millis ();
	
	EnigmaIOTNode.setLed (BLUE_LED);
	EnigmaIOTNode.setResetPin (RESET_PIN);
	EnigmaIOTNode.onConnected (connectEventHandler);
	EnigmaIOTNode.onDisconnected (disconnectEventHandler);
	EnigmaIOTNode.onDataRx (processRxData);

	EnigmaIOTNode.begin (&Espnow_hal);
	
	// Put here your code to read sensor and compose buffer
    CayenneLPP msg (MAX_DATA_PAYLOAD_SIZE);

	msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
    msg.addTemperature (1, 20.34);

	Serial.printf ("Vcc: %f\n", (float)(ESP.getVcc ()) / 1000);
	// End of user code

	Serial.printf ("Trying to send: %s\n", printHexBuffer (msg.getBuffer (), msg.getSize ()));

    // Send buffer data
	if (!EnigmaIOTNode.sendData (msg.getBuffer (), msg.getSize ())) {
		Serial.println ("---- Error sending data");
	} else {
		Serial.println ("---- Data sent");
	}
	Serial.printf ("Total time: %d ms\n", millis() - start);

    // Go to sleep
	EnigmaIOTNode.sleep ();
}

void loop () {

	EnigmaIOTNode.handle ();

}
