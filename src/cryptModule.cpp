/**
  * @file cryptModule.cpp
  * @version 0.9.7
  * @date 04/02/2021
  * @author German Martin
  * @brief Crypto library that implements EnigmaIoT encryption, decryption and key agreement fuctions
  *
  * Uses [Arduino CryptoLib](https://rweather.github.io/arduinolibs/crypto.html) library
  */

#include "cryptModule.h"
#include <Curve25519.h>
#include <ChaChaPoly.h>
#include <Poly1305.h>
#include <SHA256.h>
#include "helperFunctions.h"

CYPHER_TYPE cipher;

uint8_t* CryptModule::getSHA256 (uint8_t* buffer, uint8_t length) {
	const uint8_t HASH_LEN = 32;

	uint8_t key[HASH_LEN];

	if (length < HASH_LEN) {
		DEBUG_ERROR ("Too small buffer. Should be 32 bytes");
		return NULL;
	}

	SHA256 hash;

	// hash.reset (); // Not needed, implicit to constructor
	hash.update ((void*)buffer, length);
	hash.finalize (key, HASH_LEN);
	hash.clear ();

	/*br_sha256_context* shaContext = new br_sha256_context ();
	br_sha256_init (shaContext);
	br_sha224_update (shaContext, (void*)buffer, length);
	br_sha256_out (shaContext, key);
	delete shaContext;*/

	if (length > HASH_LEN) {
		length = HASH_LEN;
	}

	memcpy (buffer, key, length);

	return buffer;
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
			if (cipher.setIV ((uint8_t*)iv, ivlen)) {
				cipher.addAuthData ((uint8_t*)aad, aadLen);
				cipher.decrypt ((uint8_t*)data, (uint8_t*)data, length);
				bool ok = cipher.checkTag ((uint8_t*)tag, tagLen);
				cipher.clear ();
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

bool CryptModule::encryptBuffer (const uint8_t* data, size_t length,
								 const uint8_t* iv, uint8_t ivlen, const uint8_t* key, uint8_t keylen,
								 const uint8_t* aad, uint8_t aadLen, const uint8_t* tag, uint8_t tagLen) {

	if (key && iv && data) {

		DEBUG_VERBOSE ("IV: %s", printHexBuffer (iv, ivlen));
		DEBUG_VERBOSE ("Key: %s", printHexBuffer (key, keylen));
		DEBUG_VERBOSE ("AAD: %s", printHexBuffer (aad, aadLen));
		cipher.clear ();

		if (cipher.setKey ((uint8_t*)key, keylen)) {
			if (cipher.setIV ((uint8_t*)iv, ivlen)) {
				cipher.addAuthData ((uint8_t*)aad, aadLen);
				cipher.encrypt ((uint8_t*)data, (uint8_t*)data, length);
				cipher.computeTag ((uint8_t*)tag, tagLen);
				cipher.clear ();
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

uint32_t CryptModule::random () {
#ifdef ESP8266
	return *(volatile uint32_t*)RANDOM_32;
#elif defined ESP32
	return esp_random ();
#endif
}

uint8_t* CryptModule::random (const uint8_t* buf, size_t len) {
	if (buf) {
		for (unsigned int i = 0; i < len; i += sizeof (uint32_t)) {
			uint32_t rnd = random ();
			if (i < len - (len % sizeof (int32_t))) {
				memcpy (const_cast<uint8_t*>(buf) + i, &rnd, sizeof (uint32_t));
			} else {
				memcpy (const_cast<uint8_t*>(buf) + i, &rnd, len % sizeof (int32_t));
			}
		}
	}
	return const_cast<uint8_t*>(buf);
}

void CryptModule::getDH1 () {
	Curve25519::dh1 (publicDHKey, privateDHKey);
	DEBUG_VERBOSE ("Public key: %s", printHexBuffer (publicDHKey, KEY_LENGTH));

	DEBUG_VERBOSE ("Private key: %s", printHexBuffer (privateDHKey, KEY_LENGTH));
}

bool CryptModule::getDH2 (const uint8_t* remotePubKey) {
	DEBUG_VERBOSE ("Remote public key: %s", printHexBuffer (const_cast<uint8_t*>(remotePubKey), KEY_LENGTH));
	DEBUG_VERBOSE ("Private key: %s", printHexBuffer (privateDHKey, KEY_LENGTH));

	if (!Curve25519::dh2 (const_cast<uint8_t*>(remotePubKey), privateDHKey)) {
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
