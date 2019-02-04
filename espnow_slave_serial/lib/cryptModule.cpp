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
    DEBUG_MSG ("Public key: ");
    printHexBuffer (publicDHKey, KEY_LENGTH);

    DEBUG_MSG ("Private key: ");
    printHexBuffer (privateDHKey, KEY_LENGTH);
}

bool CryptModule::getDH2 (uint8_t* remotePubKey) {
    DEBUG_MSG ("Remote public key: ");
	printHexBuffer (remotePubKey, KEY_LENGTH);
	
	if (!Curve25519::dh2 (remotePubKey, privateDHKey)) {
		return false;
	}
	
    DEBUG_MSG ("Remote public key: ");
	printHexBuffer (remotePubKey, KEY_LENGTH);
    DEBUG_MSG ("Private key: ");
	printHexBuffer (privateDHKey, KEY_LENGTH);
	
	return true;
}

CryptModule Crypto;
