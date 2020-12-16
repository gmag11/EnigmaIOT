/**
  * @file timeManager.cpp
  * @version 0.9.6
  * @date 10/12/2020
  * @author German Martin
  * @brief Clock synchronisation calculations
  */

#include "timeManager.h"
#include "debug.h"

// int64_t TimeManagerClass::setOrigin () {
// 	//Serial.printf ("millis: %u\n", millis ());
// 	//Serial.printf ("offset: %d\n", offset);
// 	//Serial.printf ("clock:  %u %u\n", millis () + offset, clock());

// 	return clock ();
// }

int64_t TimeManagerClass::clock () {
    timeval currentime;
    
    gettimeofday (&currentime, NULL);
    int64_t clk = currentime.tv_sec;
    clk *= 1000000L;
    clk += currentime.tv_usec;
    // DEBUG_DBG ("Clock: %lld", clk/1000L);
    return clk/1000L;
}

int64_t TimeManagerClass::clock_us () {
    timeval currentime;

    gettimeofday (&currentime, NULL);
    int64_t clk = currentime.tv_sec;
    clk *= 1000000L;
    clk += currentime.tv_usec;
    // DEBUG_DBG ("Clock: %lld", clk);
    return clk;
}

int64_t TimeManagerClass::adjustTime (int64_t t1r, int64_t t2r, int64_t t3r, int64_t t4r) {
	int64_t t1 = t1r;
	int64_t t2 = t2r;
	int64_t t3 = t3r;
	int64_t t4 = t4r;
    timeval currenttime;
    int64_t currenttime_us;
    timeval newtime;
    int64_t newtime_us;

	DEBUG_DBG ("T1: %lld, T2: %lld, T3: %lld, T4: %lld", t1, t2, t3, t4);
	offset = ((t2 - t1) + (t3 - t4)) / 2L;
    DEBUG_DBG ("New offset: %lld", offset);
	roundTripDelay = (t4 - t1) - (t3 - t2);
    DEBUG_DBG ("Round trip delay: %lld", roundTripDelay);
    
    gettimeofday (&currenttime, NULL);
    currenttime_us = (int64_t)currenttime.tv_sec * 1000000L + (int64_t)currenttime.tv_usec;
    newtime_us = currenttime_us + offset;
    newtime.tv_sec = newtime_us / 1000000L;
    newtime.tv_usec = newtime_us - ((int64_t)(newtime.tv_sec) * 1000000L);
    
    settimeofday (&newtime, NULL); // hard adjustment

	timeIsAdjusted = true;

    return offset;
}



TimeManagerClass TimeManager;

