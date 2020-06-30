/**
  * @file timeManager.cpp
  * @version 0.9.1
  * @date 28/05/2020
  * @author German Martin
  * @brief Clock synchronisation calculations
  */

#include "timeManager.h"
#include "debug.h"

int64_t TimeManagerClass::setOrigin () {
	//Serial.printf ("millis: %u\n", millis ());
	//Serial.printf ("offset: %d\n", offset);
	//Serial.printf ("clock:  %u %u\n", millis () + offset, clock());

    return clock ();
}

int64_t TimeManagerClass::clock () {
    if (timeIsAdjusted) {
		int64_t clk = offset + (time_t)(millis ());
		DEBUG_DBG ("Clock: %lld", clk);
		return clk;
    } else {
		return millis ();
    }
}

int64_t TimeManagerClass::adjustTime (int64_t t1r, int64_t t2r, int64_t t3r, int64_t t4r) {
	int64_t t1 = t1r;
	int64_t t2 = t2r;
	int64_t t3 = t3r;
	int64_t t4 = t4r;

	DEBUG_DBG ("T1: %lld, T2: %lld, T3: %lld, T4: %lld", t1, t2, t3, t4);
	int64_t delay = ((t2 - t1) + (t3 - t4)) / 2;
	DEBUG_DBG ("Delay: %lld", delay);
    offset += delay;
	DEBUG_DBG ("New offset: %lld", offset);
    roundTripDelay = (t4 - t1) - (t3 - t2);
	DEBUG_DBG ("Round trip delay: %lld", roundTripDelay);

    timeIsAdjusted = true;

    return delay;
}



TimeManagerClass TimeManager;

