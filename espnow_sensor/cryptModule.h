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
#define BLOCK_SIZE 16U
#define BUFFER_SIZE 255U

class CryptModule {
public:
    static uint32_t random ();

    void getDH1 ();

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

