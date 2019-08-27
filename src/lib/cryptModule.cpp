/**
  * @file cryptModule.cpp
  * @version 0.2.0
  * @date 28/06/2019
  * @author German Martin
  * @brief Crypto library that implements EnigmaIoT encryption, decryption and key agreement fuctions
  *
  * Uses [Arduino CryptoLib](https://rweather.github.io/arduinolibs/crypto.html) library
  */

//#include <CFB.h>
//#include <CryptoLW.h>
#include "cryptModule.h"
//#include <Speck.h>
#include <Curve25519.h>
#include <ChaChaPoly.h>
#include <Poly1305.h>
#include "helperFunctions.h"

//#define BLOCK_CYPHER Speck
//#define CYPHER_TYPE CFB<BLOCK_CYPHER>

CYPHER_TYPE cipher;
//BLOCK_CYPHER netCipher;

uint8_t *CryptModule::getSHA256FromKey (uint8_t* inputKey, uint8_t keyLength) {
	uint8_t key[32];
	
	br_sha256_context* shaContext = new br_sha256_context ();
	br_sha256_init (shaContext);
	br_sha224_update (shaContext, (void*)inputKey, keyLength);
	br_sha256_out (shaContext, key);
	delete shaContext;

	memcpy (inputKey, key, keyLength);

	return inputKey;
}

bool CryptModule::decryptBuffer (const uint8_t* data, size_t length,
                                 const uint8_t* iv, uint8_t ivlen, const uint8_t* key, uint8_t keylen,
                                 const uint8_t* aad, uint8_t aadLen, const uint8_t* tag, uint8_t tagLen) {
    if (key && iv && data) {
        
        DEBUG_VERBOSE ("IV: %s", printHexBuffer (iv, ivlen));
        DEBUG_VERBOSE ("Key: %s", printHexBuffer (key, keylen));
        DEBUG_VERBOSE ("AAD: %s", printHexBuffer (aad, aadLen));
        cipher.clear ();
        
        if (cipher.setKey (key, keylen)) {
            if (cipher.setIV (iv, ivlen)) {
                cipher.addAuthData (aad, aadLen);
                cipher.decrypt (output, input, length);
                bool ok = cipher.checkTag (tag, tagLen);
                DEBUG_VERBOSE ("Tag: %s", printHexBuffer (tag, tagLen));
                if (!ok) {
                    DEBUG_ERROR ("Data authentication error");
                }
                return ok;
            } else {
                DEBUG_ERROR ("Error setting IV");
            }
        } else {
            DEBUG_ERROR ("Error setting key");
        }
    } else {
        DEBUG_ERROR ("Error in key or IV");
    }

    return false;
}

bool CryptModule::encryptBuffer (const uint8_t *data, size_t length,
                                 const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen, 
                                 const uint8_t *aad, uint8_t aadLen, const uint8_t* tag, uint8_t tagLen) {
    
    if ( key && iv && data ) {

        DEBUG_VERBOSE ("IV: %s", printHexBuffer (iv, ivlen));
        DEBUG_VERBOSE ("Key: %s", printHexBuffer (key, keylen));
        DEBUG_VERBOSE ("AAD: %s", printHexBuffer (aad, aadLen));
        cipher.clear ();
        
        if (cipher.setKey (key, keylen)) {
            if (cipher.setIV (iv, ivlen)) {
                cipher.addAuthData(aad,aadLen);
                cipher.encrypt (output, data, length);
                cipher.computeTag(tag, tagLen);
                cipher.clear();
                DEBUG_VERBOSE ("Tag: %s", printHexBuffer (tag, tagLen));
                return true;
            } else {
                DEBUG_ERROR ("Error setting IV");
            }
        } else {
            DEBUG_ERROR ("Error setting key");
        }
    } else {
        DEBUG_ERROR ("Error on input data for encryption");
    }

    return false;

}

//uint8_t* CryptModule::networkEncrypt (uint8_t* input, size_t inputLen, uint8_t* key, size_t keyLen, uint8_t* tag) {
//
//    
//    uint8_t *tempBuffer = input;
//
//    netCipher.setKey (key, keyLength);
//    size_t blockSize = netCipher.blockSize ();
//
//    for (int i = 0; i < numBlocks; i++) {
//        netCipher.encryptBlock (tempBuffer, tempBuffer);
//        tempBuffer += blockSize;
//    }
//    return input;
//}
//
//uint8_t* CryptModule::networkDecrypt (uint8_t* input, uint8_t numBlocks, uint8_t* key, uint8_t keyLength, uint8_t *tag) {
//
//    uint8_t *tempBuffer = input;
//
//    netCipher.setKey (key, keyLength);
//    size_t blockSize = netCipher.blockSize ();
//
//    for (int i = 0; i < numBlocks; i++) {
//        netCipher.decryptBlock (tempBuffer, tempBuffer);
//        tempBuffer += blockSize;
//    }
//    return input;
//}


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
