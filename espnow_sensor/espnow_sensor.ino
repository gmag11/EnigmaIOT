#include <WifiEspNow.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "cryptModule.h"
#include "helperFunctions.h"



void initWiFi () {
    WiFi.persistent (false);
    WiFi.mode (WIFI_AP);
    WiFi.softAP ("ESPNOW", nullptr, 3);
    WiFi.softAPdisconnect (false);

    Serial.print ("MAC address of this node is ");
    Serial.println (WiFi.softAPmacAddress ());
}

void initEspNow () {
    bool ok = WifiEspNow.begin ();
    if (!ok) {
        Serial.println ("WifiEspNow.begin() failed");
        ESP.restart ();
    }
}

void setup () {
    //***INICIALIZACIÓN DEL PUERTO SERIE***//
    Serial.begin (115200); Serial.println (); Serial.println ();

    initWiFi ();
    initEspNow ();

    Crypto.getDH1 ();
}

void loop () {

}