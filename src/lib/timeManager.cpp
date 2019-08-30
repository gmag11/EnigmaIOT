// 
// 
// 

#include "timeManager.h"

clock_t TimeManagerClass::setOrigin () {
    t1 = getClock ();
    return t1;
}

clock_t TimeManagerClass::getClock () {
    if (timeIsAdjusted) {
        return millis () + offset;
    } else {
        return millis ();
    }
}

time_t TimeManagerClass::adjustTime (clock_t t2r, clock_t t3r, clock_t t4r) {
    t2 = t2r;
    t3 = t3r;
    t4 = t4r;

    offset = (long)((t2 - t1) + (t3 - t4)) / 2L;
    roundTripDelay = (t4 - t1) - (t3 - t2);

    timeIsAdjusted = true;

    return offset;
}



TimeManagerClass TimeManager;

