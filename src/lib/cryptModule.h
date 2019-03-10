// cryptModule.h

#ifndef _CRYPTMODULE_h
#define _CRYPTMODULE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define RANDOM_32 0x3FF20E44
#define KEY_LENGTH 32U
#define IV_LENGTH 16U
#define RANDOM_LENGTH sizeof(uint32_t)
#define CRC_LENGTH sizeof(uint32_t)
//#define BLOCK_SIZE 16U
#define BUFFER_SIZE 255U

class CryptModule {
public:
    static uint32_t random ();
    static uint8_t *random (const uint8_t *buf, size_t len);
    static void decryptBuffer (uint8_t *output, const uint8_t *input, size_t length,
        const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen);
    static void encryptBuffer (uint8_t *output, const uint8_t *input, size_t length,
        const uint8_t *iv, uint8_t ivlen, const uint8_t *key, uint8_t keylen);

    void getDH1 ();
	bool getDH2 (const uint8_t* remotePubKey);

    uint8_t* getPrivDHKey () {
        return privateDHKey;
    }

    uint8_t* getPubDHKey () {
        return publicDHKey;
    }
    
    //static size_t getBlockSize ();


protected:
    uint8_t privateDHKey[KEY_LENGTH];
    uint8_t publicDHKey[KEY_LENGTH];
};

extern CryptModule Crypto;

#endif

