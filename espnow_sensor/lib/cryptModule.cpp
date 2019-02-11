// 
// 
// 
#include <Crypto.h>
#include <CFB.h>
#include <CryptoLW.h>
#include "cryptModule.h"
#include <Speck.h>
#include <Curve25519.h>
#include "helperFunctions.h"

#define CYPHER_TYPE Speck

CFB<CYPHER_TYPE> cfb;

void CryptModule::decryptBuffer (uint8_t *output, uint8_t *input, size_t length,
    uint8_t *iv, uint8_t ivlen, uint8_t *key, uint8_t keylen) {
    if (key && iv) {
        if (cfb.setKey (key, keylen)) {
            if (cfb.setIV (iv, ivlen)) {
                cfb.decrypt (output, input, length);
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

/*void CryptModule::decryptBuffer (byte *output, byte *input, size_t length, byte *key) {
	CYPHER_TYPE cipher;
	size_t blockSize = cipher.blockSize ();
	size_t keySize = cipher.keySize ();
	int index = 0;
	uint8_t numBlocks;

	if (length % blockSize == 0) {
		numBlocks = (uint8_t)(length / blockSize);
	}
	else {
		numBlocks = (uint8_t)(length / blockSize) + 1;
	}

	DEBUG_VERBOSE ("BufferLength = %d. numBlocks = %u", length, numBlocks);
	//DEBUG_VERBOSE ("Decryption key: %s", printHexBuffer(key,KEY_LENGTH));
	cipher.setKey (key, keySize);

	for (int i = 0; i < numBlocks; i++) {
		// TODO: Check buffer limit.

		//Serial.printf ("%s Before data: ", __FUNCTION__);
		//printKey (&buffer[index], blockSize); Serial.println ();

		cipher.decryptBlock (&output[index], &input[index]);

		index += blockSize;
	}

	cipher.clear ();
}*/

void CryptModule::encryptBuffer (uint8_t *output, uint8_t *input, size_t length,
    uint8_t *iv, uint8_t ivlen, uint8_t *key, uint8_t keylen) {
    if (key && iv) {
        if (cfb.setKey (key, keylen)) {
            if (cfb.setIV (iv, ivlen)) {
                cfb.encrypt (output, input, length);
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

/*void CryptModule::encryptBuffer (byte *output, byte *input, size_t length, byte *key) {
    CYPHER_TYPE cipher;
    size_t blockSize = cipher.blockSize ();
    size_t keySize = cipher.keySize ();
    int index = 0;
    uint8_t numBlocks;

    if (length % blockSize == 0) {
        numBlocks = (uint8_t)(length / blockSize);
    }
    else {
        numBlocks = (uint8_t)(length / blockSize) + 1;
    }

    DEBUG_VERBOSE ("BufferLength = %d. numBlocks = %u. BlockSize = %u",
        length, numBlocks, blockSize);
    //DEBUG_VERBOSE ("Encryption key: %s", printHexBuffer (key, KEY_LENGTH));

    cipher.setKey (key, keySize);

    for (int i = 0; i < numBlocks; i++) {
        // TODO: Check buffer limit.

        //Serial.printf ("%s Before data: ", __FUNCTION__);
        //printKey (&buffer[index], blockSize); Serial.println ();

        cipher.encryptBlock (&output[index], &input[index]);

        index += blockSize;
    }

    cipher.clear ();
}*/

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

size_t CryptModule::getBlockSize ()
{
    CYPHER_TYPE cipher;
    return cipher.blockSize();
}

CryptModule Crypto;
