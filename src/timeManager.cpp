/**
  * @file timeManager.cpp
  * @version 0.8.3
  * @date 05/05/2020
  * @author German Martin
  * @brief Clock synchronisation calculations
  */

#include "timeManager.h"

clock_t TimeManagerClass::setOrigin () {
	//Serial.printf ("millis: %u\n", millis ());
	//Serial.printf ("offset: %d\n", offset);
	//Serial.printf ("clock:  %u %u\n", millis () + offset, clock());

    return clock ();
}

clock_t TimeManagerClass::clock () {
    if (timeIsAdjusted) {
		clock_t clk = (clock_t)(offset + (time_t)(millis ()));
		return clk;
    } else {
		return millis ();
    }
}

time_t TimeManagerClass::adjustTime (clock_t t1r, clock_t t2r, clock_t t3r, clock_t t4r) {
	clock_t t1 = t1r;
	clock_t t2 = t2r;
	clock_t t3 = t3r;
	clock_t t4 = t4r;

	time_t delay = (long)((t2 - t1) + (t3 - t4)) / 2L;
    offset += delay;
    roundTripDelay = (t4 - t1) - (t3 - t2);

    timeIsAdjusted = true;

    return delay;
}



TimeManagerClass TimeManager;

