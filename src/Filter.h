// Filter.h

#ifndef _FILTER_h
#define _FILTER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define MAX_ORDER 20
#define MIN_ORDER 2

typedef enum {
	MEDIAN_FILTER,
	AVERAGE_FILTER
} FilterType_t;

class FilterClass
{
 protected:
	 FilterType_t _filterType;
	 uint8_t _order;
	 float* _rawValues;
	 float* _orderedValues;
	 float* _weightValues;
	 uint _index = 0;
	 float aveFilter(float value);
	 int divide(float * array, int start, int end);
	 void quicksort(float * array, int start, int end);
	 float medianFilter(float value);

 public:
	FilterClass(FilterType_t type, uint8_t order);
	float addWeigth (float coeff);
	float addValue(float value);
	void clear ();
	~FilterClass();
};

extern FilterClass Filter;

#endif

