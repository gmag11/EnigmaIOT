// 
// 
// 
#include <CryptoLW.h>
#include "cryptModule.h"
#include <Speck.h>
#include <Curve25519.h>
#include "helperFunctions.h"

#define CYPHER_TYPE Speck

void CryptModule::decryptBuffer (byte *output, byte *input, size_t length, byte *key) {
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
}

void CryptModule::encryptBuffer (byte *output, byte *input, size_t length, byte *key) {
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
}

uint32_t CryptModule::random () {
    return *(volatile uint32_t *)RANDOM_32;
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
