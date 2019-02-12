// 
// 
// 
//#include <Crypto.h>
#include <CFB.h>
#include <CryptoLW.h>
#include "cryptModule.h"
#include <Speck.h>
#include <Curve25519.h>
#include "helperFunctions.h"

#define CYPHER_TYPE CFB<Speck>

CYPHER_TYPE cipher;

void CryptModule::decryptBuffer (uint8_t *output, uint8_t *input, size_t length,
    uint8_t *iv, uint8_t ivlen, uint8_t *key, uint8_t keylen) {
    if (key && iv) {
        if (cipher.setKey (key, keylen)) {
            if (cipher.setIV (iv, ivlen)) {
                cipher.decrypt (output, input, length);
            } else {
                DEBUG_ERROR ("Error setting IV");
            }
        } else {
            DEBUG_ERROR ("Error setting key");
        }
    } else {
        DEBUG_ERROR ("Error in key or IV");
    }
}

void CryptModule::encryptBuffer (uint8_t *output, uint8_t *input, size_t length,
    uint8_t *iv, uint8_t ivlen, uint8_t *key, uint8_t keylen) {
    if (key && iv) {
        if (cipher.setKey (key, keylen)) {
            if (cipher.setIV (iv, ivlen)) {
                cipher.encrypt (output, input, length);
            } else {
                DEBUG_ERROR ("Error setting IV");
            }
        } else {
            DEBUG_ERROR ("Error setting key");
        }
    } else {
        DEBUG_ERROR ("Error in key or IV");
    }
}

uint32_t CryptModule::random () {
    return *(volatile uint32_t *)RANDOM_32;
}

uint8_t *CryptModule::random (uint8_t *buf, size_t len) {
    if (buf) {
        for (int i = 0; i < len; i += sizeof (uint32_t)) {
            uint32 rnd = random ();
            if (i < len - (len % sizeof (int32_t))) {
                memcpy (buf + i, &rnd, sizeof (uint32_t));
            } else {
                memcpy (buf + i, &rnd, len % sizeof (int32_t));
            }
        }
    }
    return buf;
}

void CryptModule::getDH1 () {
    Curve25519::dh1 (publicDHKey, privateDHKey);
	DEBUG_VERBOSE ("Public key: %s", printHexBuffer (publicDHKey, KEY_LENGTH));

	DEBUG_VERBOSE ("Private key: %s", printHexBuffer (privateDHKey, KEY_LENGTH));
}

bool CryptModule::getDH2 (uint8_t* remotePubKey) {
	DEBUG_VERBOSE ("Remote public key: %s", printHexBuffer (remotePubKey, KEY_LENGTH));
	DEBUG_VERBOSE ("Private key: %s", printHexBuffer (privateDHKey, KEY_LENGTH));

	if (!Curve25519::dh2 (remotePubKey, privateDHKey)) {
		DEBUG_WARN ("DH2 error");
		return false;
	}
	
	return true;
}

/*size_t CryptModule::getBlockSize ()
{
    CYPHER_TYPE cipher;
    return cipher.blockSize();
}*/

CryptModule Crypto;
