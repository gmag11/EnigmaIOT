/**
  * @file enigmaiot_gateway.ino
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief Gateway based on EnigmaIoT over ESP-NOW
  *
  * Communicates with a serial to MQTT gateway to send data to any IoT platform
  */

#include <CayenneLPPDec.h>
#include <EnigmaIOTGateway.h>
#include <espnow_hal.h>


#define BLUE_LED 2
#define RED_LED 16

void processRxData (const uint8_t* mac, const uint8_t* buffer, uint8_t length, uint16_t lostMessages) {
    StaticJsonBuffer<256> jsonBuffer;
    JsonArray& root = jsonBuffer.createArray ();

    char macstr[18];
    mac2str (mac, macstr);
    //Serial.printf ("Data from %s --> %s\n", macstr, printHexBuffer (buffer, length));

    CayenneLPPDec::ParseLPP (buffer, length, root);
    //root.prettyPrintTo (Serial);
    //Serial.println ();
    Serial.printf ("~/%s/data;", macstr);
    root.printTo (Serial);

    Serial.println ();
    if (lostMessages > 0) {
        //serial.printf ("%u lost messages\n", lostmessages);
        Serial.printf ("~/%s/debug/lostmessages;%u\n", macstr, lostMessages);
    }
    Serial.printf ("~/%s/status/{\"per\":%e,\"lostmessages\":%u,\"totalmessages\":%u,\"packetshour\":%f}\n",
        macstr, 
        EnigmaIOTGateway.getPER((uint8_t*)mac), 
        EnigmaIOTGateway.getErrorPackets((uint8_t*)mac),
        EnigmaIOTGateway.getTotalPackets((uint8_t*)mac),
        EnigmaIOTGateway.getPacketsHour((uint8_t*)mac));
    //Serial.println ();
}

void newNodeConnected (uint8_t* mac) {
    char macstr[18];
    mac2str (mac, macstr);
    //Serial.printf ("New node connected: %s\n", macstr);
    Serial.printf ("~/%s/hello\n", macstr);
}

void nodeDisconnected (uint8_t* mac, gwInvalidateReason_t reason) {
    char macstr[18];
    mac2str (mac, macstr);
    //Serial.printf ("Node %s disconnected. Reason %u\n", macstr, reason);
    Serial.printf ("~/%s/bye;{\"reason\":%u}\n", macstr, reason);

}

void setup () {
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    EnigmaIOTGateway.setRxLed (BLUE_LED);
    EnigmaIOTGateway.setTxLed (RED_LED);
    EnigmaIOTGateway.onNewNode (newNodeConnected);
    EnigmaIOTGateway.onNodeDisconnected (nodeDisconnected);
    EnigmaIOTGateway.begin (&Espnow_hal, (uint8_t*)NETWORK_KEY);
    EnigmaIOTGateway.onDataRx (processRxData);
}

void loop () {
    EnigmaIOTGateway.handle ();
}