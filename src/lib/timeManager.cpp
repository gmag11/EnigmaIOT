// 
// 
// 

#include "timeManager.h"

clock_t TimeManagerClass::setOrigin () {
	Serial.printf ("millis: %u\n", millis ());
	Serial.printf ("offset: %d\n", offset);
	Serial.printf ("clock:  %u %u\n", millis () + offset, clock());

    t1 = clock ();
    return t1;
}

clock_t TimeManagerClass::clock () {
    if (timeIsAdjusted) {
		clock_t clk = (clock_t)(offset + (time_t)(millis ()));
		return clk;
    } else {
        return millis ();
    }
}

time_t TimeManagerClass::adjustTime (clock_t t2r, clock_t t3r, clock_t t4r) {
    t2 = t2r;
    t3 = t3r;
    t4 = t4r;

	time_t delay = (long)((t2 - t1) + (t3 - t4)) / 2L;
    offset += delay;
    roundTripDelay = (t4 - t1) - (t3 - t2);

    timeIsAdjusted = true;

    return delay;
}



TimeManagerClass TimeManager;

