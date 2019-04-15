/**
  * @file cryptModule.h
  * @version 0.0.1
  * @date 09/03/2019
  * @author German Martin
  * @brief Crypto library that implements EnigmaIoT encryption, decryption and key agreement fuctions
  *
  * Uses [Arduino CryptoLib](https://rweather.github.io/arduinolibs/crypto.html) library
  */

#ifndef _CRYPTMODULE_h
#define _CRYPTMODULE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define RANDOM_32 0x3FF20E44
const uint8_t KEY_LENGTH = 32; ///< @brief Key length used by selected crypto algorythm
const uint8_t IV_LENGTH = 16; ///< @brief Initalization vector length used by selected crypto algorythm
const uint8_t RANDOM_LENGTH = sizeof (uint32_t); ///< @brief Length of random number generator values
const uint8_t CRC_LENGTH = sizeof (uint32_t); ///< @brief Length of CRC
//#define BLOCK_SIZE 16U
//const uint8_t BUFFER_SIZE = 255; 

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

    /**
      * @brief Fills a buffer with random values
      * @param buf Pointer to the buffer to fill
      * @param len Buffer length in number of bytes
      * @return Returns the same buffer used as input, filled with random data
      */
    static uint8_t *random (const uint8_t *buf, size_t len);

    /**
      * @brief Decrypts a buffer using a shared key
      * @param output Output buffer to get decrypted data. It may be the same as input buffer
      * @param input Input encrypted data buffer
      * @param length Buffer length in number of bytes
      * @param iv Initialization Vector used to encrypt this data
      * @param ivlen IV length
      * @param key Shared key used to encrypt data
      * @param keylen Key length
      */
    static void decryptBuffer (uint8_t *output, const uint8_t *input, size_t length,
        const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen);

    /**
      * @brief Encrypts a buffer using a shared key
      * @param output Output buffer to get encrypted data. It may be the same as input buffer
      * @param input Input clear data buffer
      * @param length Buffer length in number of bytes
      * @param iv Initialization Vector to be used to encrypt input data
      * @param ivlen IV length
      * @param key Shared key to be used to encrypt data
      * @param keylen Key length
      */
    static void encryptBuffer (uint8_t *output, const uint8_t *input, size_t length,
        const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen);

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

    /**
      * @brief Sets network key used to protect key agreement against man in the middle and gateway or node spoofing
      * @param key Pointer to network key
      */
    void setNetworkKey (uint8_t* key) {
        memcpy (networkKey, key, KEY_LENGTH);
    }

    /**
      * @brief Gets network key used to protect key agreement against man in the middle and gateway or node spoofing
      * @return Pointer to network key
      */
    uint8_t* getNetworkKey () {
        return networkKey;
    }
    
    //static size_t getBlockSize ();


protected:
    uint8_t privateDHKey[KEY_LENGTH]; ///< @brief Temporary private key store used during key agreement
    uint8_t publicDHKey[KEY_LENGTH];  ///< @brief Temporary public key store used during key agreement
    uint8_t networkKey[KEY_LENGTH];   ///< @brief Network key to protect key agreement
};

extern CryptModule Crypto; ///< @brief Singleton Crypto class instance

#endif

