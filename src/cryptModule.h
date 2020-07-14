/**
  * @file cryptModule.h
  * @version 0.9.3
  * @date 14/07/2020
  * @author German Martin
  * @brief Crypto library that implements EnigmaIoT encryption, decryption and key agreement fuctions
  *
  * Uses [Arduino CryptoLib](https://rweather.github.io/arduinolibs/crypto.html) library
  */

#ifndef _CRYPTMODULE_h
#define _CRYPTMODULE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "EnigmaIoTconfig.h"

#define CRYPTMODULE_DEBUG_TAG "CryptModule"

#ifdef ESP8266
#define RANDOM_32 0x3FF20E44
#endif

const uint8_t RANDOM_LENGTH = sizeof (uint32_t); ///< @brief Length of random number generator values
const uint8_t CRC_LENGTH = sizeof (uint32_t); ///< @brief Length of CRC

/**
  * @brief EnigmaIoT Crypto module. Wraps Arduino CryptoLib classes and methods
  *
  * Uses [Arduino CryptoLib](https://rweather.github.io/arduinolibs/crypto.html) library
  */
class CryptModule {
public:
	/**
	  * @brief Gets a random number
	  * @return Returns a random number
	  */
	static uint32_t random ();

	static uint32_t random (uint32_t max, uint32_t min = 0) {
		uint32_t _max, _min;

		if (max > min) {
			_max = max;
			_min = min;
		} else {
			_max = min;
			_min = max;
		}

		return _min + (random () % (_max - _min));
	}

	/**
	  * @brief Fills a buffer with random values
	  * @param buf Pointer to the buffer to fill
	  * @param len Buffer length in number of bytes
	  * @return Returns the same buffer used as input, filled with random data
	  */
	static uint8_t* random (const uint8_t* buf, size_t len);

	/**
	  * @brief Decrypts a buffer using a shared key
	  * @param data Buffer to decrypt. It will be used as input and output
	  * @param length Buffer length in number of bytes
	  * @param iv Initialization Vector used to encrypt this data
	  * @param ivlen IV length
	  * @param key Shared key used to encrypt data
	  * @param keylen Key length
	  * @param aad Additional Authentication Data for Poly1305
	  * @param aadLen Additional Authentication Data length
	  * @param tag Buffer to store authentication tag calculated by Poly1305
	  * @param tagLen Additional Authentication Tag length
	  * @return True if decryption and tag checking was correct
	  */
	static bool decryptBuffer (const uint8_t* data, size_t length,
							   const uint8_t* iv, uint8_t ivlen, const uint8_t* key, uint8_t keylen,
							   const uint8_t* aad, uint8_t aadLen, const uint8_t* tag, uint8_t tagLen);

	/**
	  * @brief Generates a SHA256 hash from input
	  * @param buffer Buffer with data to hash. Hash will be stored here
	  * @param length Buffer length in number of bytes. It should be 32 at least
	  * @return Returns buffer pointer
	  */
	static uint8_t* getSHA256 (uint8_t* buffer, uint8_t length);

	/**
	  * @brief Decrypts a buffer using a shared key
	  * @param data Buffer to decrypt. It will be used as input and output
	  * @param length Buffer length in number of bytes
	  * @param iv Initialization Vector used to encrypt this data
	  * @param ivlen IV length
	  * @param key Shared key used to encrypt data
	  * @param keylen Key length
	  * @param aad Additional Authentication Data for Poly1305
	  * @param aadLen Additional Authentication Data length
	  * @param tag Buffer to store authentication tag calculated by Poly1305
	  * @param tagLen Additional Authentication Tag length
	  * @return True if encryption and tag generation was correct
	  */
	static bool encryptBuffer (const uint8_t* data, size_t length,
							   const uint8_t* iv, uint8_t ivlen, const uint8_t* key, uint8_t keylen,
							   const uint8_t* aad, uint8_t aadLen, const uint8_t* tag, uint8_t tagLen);

	/**
	  * @brief Starts first stage of Diffie Hellman key agreement algorithm
	  */
	void getDH1 ();

	/**
	  * @brief Starts second stage of Diffie Hellman key agreement algorithm and calculate shares key
	  * @param remotePubKey Public key got from the other peer
	  * @return `true` if calculation worked normally, `false` otherwise.
	  */
	bool getDH2 (const uint8_t* remotePubKey);

	/**
	  * @brief Gets own private key used on Diffie Hellman algorithm
	  * @return Pointer to private key
	  */
	uint8_t* getPrivDHKey () {
		return privateDHKey;
	}

	/**
	  * @brief Gets own public key used on Diffie Hellman algorithm
	  * @return Pointer to public key
	  */
	uint8_t* getPubDHKey () {
		return publicDHKey;
	}

protected:
	uint8_t privateDHKey[KEY_LENGTH]; ///< @brief Temporary private key store used during key agreement
	uint8_t publicDHKey[KEY_LENGTH];  ///< @brief Temporary public key store used during key agreement
};

extern CryptModule Crypto; ///< @brief Singleton Crypto class instance

#endif

