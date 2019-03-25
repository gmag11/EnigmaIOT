/**
  * @file enigmaiot_gateway.ino
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  */

#include <CayenneLPP.h>
#include <CayenneLPPDec.h>
#include <EnigmaIOTGateway.h>
#include <espnow_hal.h>


#define BLUE_LED 2
#define RED_LED 16

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages) {
    char macstr[18];
    mac2str (mac, macstr);
    Serial.println ();
    Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));
    for (int i = 0; i < length; i++) {
        Serial.print ((char)buffer[i]);
    }
    Serial.println ();
    if (lostMessages > 0) {
        Serial.printf ("%u lost messages\n", lostMessages);
    }
    Serial.println ();
    EnigmaIOTGateway.sendDownstream ((uint8_t *)mac, (uint8_t *)"ACK", 4);
}

void newNodeConnected (uint8_t* mac) {
    char macstr[18];
    mac2str (mac, macstr);
    Serial.printf ("New node connected: %s\n", macstr);
}

void nodeDisconnected (uint8_t* mac, gwInvalidateReason_t reason) {
    char macstr[18];
    mac2str (mac, macstr);
    Serial.printf ("Node %s disconnected. Reason %u\n", macstr, reason);
}

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    EnigmaIOTGateway.setRxLed (BLUE_LED);
    EnigmaIOTGateway.setTxLed (RED_LED);
    EnigmaIOTGateway.onNewNode (newNodeConnected);
    EnigmaIOTGateway.onNodeDisconnected (nodeDisconnected);
    EnigmaIOTGateway.begin (&Espnow_hal);
    EnigmaIOTGateway.onDataRx (processRxData);
}

void loop () {
    EnigmaIOTGateway.handle ();
}