/**
  * @file timeManager.h
  * @version 0.4.0
  * @date 10/09/2019
  * @author German Martin
  * @brief Clock synchronisation calculations
  */

#ifndef _TIMEMANAGER_h
#define _TIMEMANAGER_h

#include <Arduino.h>

class TimeManagerClass {
protected:
    //clock_t t1, t2, t3, t4;
    bool timeIsAdjusted = false;
    time_t offset;
    time_t roundTripDelay;

public:
    clock_t setOrigin ();

    clock_t clock ();

    time_t adjustTime (clock_t t1r, clock_t t2r, clock_t t3r, clock_t t4r);

    time_t getOffset () {
        return offset;
    }

    bool isTimeAdjusted () {
        return timeIsAdjusted;
    }

    time_t getDelay () {
        return roundTripDelay;
    }
};

extern TimeManagerClass TimeManager;

#endif

