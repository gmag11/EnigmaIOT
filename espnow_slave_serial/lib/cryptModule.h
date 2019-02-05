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
#define RANDOM_LENGTH 4U
#define CRC_LENGTH 4U
#define BLOCK_SIZE 16U
#define BUFFER_SIZE 255U

class CryptModule {
public:
    static uint32_t random ();
	static void decryptBuffer (byte *output, byte *input, size_t length, byte *key);
	static void encryptBuffer (byte *output, byte *input, size_t length, byte *key);

    void getDH1 ();
	bool getDH2 (uint8_t* remotePubKey);

    byte* getPrivDHKey () {
        return privateDHKey;
    }

    byte* getPubDHKey () {
        return publicDHKey;
    }


protected:
    byte privateDHKey[KEY_LENGTH];
    byte publicDHKey[KEY_LENGTH];
};

extern CryptModule Crypto;

#endif

