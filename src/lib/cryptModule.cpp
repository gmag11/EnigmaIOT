/**
  * @file cryptModule.cpp
  * @version 0.1.0
  * @date 09/03/2019
  * @author German Martin
  * @brief Crypto library that implements EnigmaIoT encryption, decryption and key agreement fuctions
  *
  * Uses [Arduino CryptoLib](https://rweather.github.io/arduinolibs/crypto.html) library
  */

#include <CFB.h>
#include <CryptoLW.h>
#include "cryptModule.h"
#include <Speck.h>
#include <Curve25519.h>
#include "helperFunctions.h"

#define BLOCK_CYPHER Speck
#define CYPHER_TYPE CFB<BLOCK_CYPHER>

CYPHER_TYPE cipher;
BLOCK_CYPHER netCipher;

void CryptModule::decryptBuffer (uint8_t *output, const uint8_t *input, size_t length,
    const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen) {
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

void CryptModule::encryptBuffer (uint8_t *output, const uint8_t *input, size_t length,
    const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen) {
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

uint8_t* CryptModule::networkEncrypt (uint8_t* input, uint8_t numBlocks, uint8_t* key, uint8_t keyLength) {

    uint8_t *tempBuffer = input;

    netCipher.setKey (key, keyLength);
    size_t blockSize = netCipher.blockSize ();

    for (int i = 0; i < numBlocks; i++) {
        netCipher.encryptBlock (tempBuffer, tempBuffer);
        tempBuffer += blockSize;
    }
    return input;
}

uint8_t* CryptModule::networkDecrypt (uint8_t* input, uint8_t numBlocks, uint8_t* key, uint8_t keyLength) {

    uint8_t *tempBuffer = input;

    netCipher.setKey (key, keyLength);
    size_t blockSize = netCipher.blockSize ();

    for (int i = 0; i < numBlocks; i++) {
        netCipher.decryptBlock (tempBuffer, tempBuffer);
        tempBuffer += blockSize;
    }
    return input;
}


uint32_t CryptModule::random () {
    return *(volatile uint32_t *)RANDOM_32;
}

uint8_t *CryptModule::random (const uint8_t *buf, size_t len) {
    if (buf) {
        for (unsigned int i = 0; i < len; i += sizeof (uint32_t)) {
            uint32 rnd = random ();
            if (i < len - (len % sizeof (int32_t))) {
                memcpy (const_cast<uint8_t *>(buf) + i, &rnd, sizeof (uint32_t));
            } else {
                memcpy (const_cast<uint8_t *>(buf) + i, &rnd, len % sizeof (int32_t));
            }
        }
    }
    return const_cast<uint8_t *>(buf);
}

void CryptModule::getDH1 () {
    Curve25519::dh1 (publicDHKey, privateDHKey);
	DEBUG_VERBOSE ("Public key: %s", printHexBuffer (publicDHKey, KEY_LENGTH));

	DEBUG_VERBOSE ("Private key: %s", printHexBuffer (privateDHKey, KEY_LENGTH));
}

bool CryptModule::getDH2 (const uint8_t* remotePubKey) {
	DEBUG_VERBOSE ("Remote public key: %s", printHexBuffer (const_cast<uint8_t *>(remotePubKey), KEY_LENGTH));
	DEBUG_VERBOSE ("Private key: %s", printHexBuffer (privateDHKey, KEY_LENGTH));

	if (!Curve25519::dh2 (const_cast<uint8_t *>(remotePubKey), privateDHKey)) {
		DEBUG_WARN ("DH2 error");
		return false;
	}
    memset (publicDHKey, 0, KEY_LENGTH); // delete public key from memory
	
	return true;
}

/*size_t CryptModule::getBlockSize ()
{
    CYPHER_TYPE cipher;
    return cipher.blockSize();
}*/

CryptModule Crypto;
