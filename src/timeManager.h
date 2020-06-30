/**
  * @file timeManager.h
  * @version 0.9.1
  * @date 28/05/2020
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
    int64_t offset = 0;
    int64_t roundTripDelay;

public:
    int64_t setOrigin ();

    int64_t clock ();

    time_t unixtime () {
        uint64_t time_sec = clock () / 1000;
        return time_sec;
    }

    int64_t adjustTime (int64_t t1r, int64_t t2r, int64_t t3r, int64_t t4r);

    int64_t getOffset () {
        return offset;
    }

    bool isTimeAdjusted () {
        return timeIsAdjusted;
    }

    int64_t getDelay () {
        return roundTripDelay;
    }

	void reset () {
		offset = 0;
		timeIsAdjusted = false;
	}
};

extern TimeManagerClass TimeManager;

#endif

