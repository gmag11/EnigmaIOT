// 
// 
// 
#include <CryptoLW.h>
#include "cryptModule.h"
#include <Speck.h>
#include <Curve25519.h>
#include "helperFunctions.h"


uint32_t CryptModule::random () {
    return *(volatile uint32_t *)RANDOM_32;
}


void CryptModule::getDH1 () {
    Curve25519::dh1 (publicDHKey, privateDHKey);
    Serial.printf ("[%s] Public key: ", __FUNCTION__);
    printHexBuffer (publicDHKey, KEY_LENGTH);

    Serial.printf ("[%s] Private key: ", __FUNCTION__);
    printHexBuffer (privateDHKey, KEY_LENGTH);
}

CryptModule Crypto;
