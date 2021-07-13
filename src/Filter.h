/**
  * @file Filter.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief Filter to process message rate or other values
  */

#ifndef _FILTER_h
#define _FILTER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define MAX_ORDER 20
#define MIN_ORDER 2

/**
  * @brief Type of filter
  */
typedef enum {
	MEDIAN_FILTER, /**< Median filter */
	AVERAGE_FILTER /**< Average filter */
} FilterType_t;

class FilterClass {
protected:
	FilterType_t _filterType; ///< @brief Filter type from FilterType_t
	uint8_t _order; ///< @brief Filter order. Numbre of samples to store for calculations
	float* _rawValues; ///< @brief Raw values store
	float* _orderedValues; ///< @brief Values ordered for median calculation
	float* _weightValues; ///< @brief Weight values for average calculation. By default all them have value of 1 for arithmetic average
	uint _index = 0;///< @brief Used to point latest entered value while number of values less than order

	/**
	 * @brief Average filter calculation of next value
	 * @param value Next value to do calculation with
	 * @return Returns calculated average (weighted or unweighted)
	 */
	float aveFilter (float value);

	/**
	 * @brief Divide function to be used on Quick Sort
	 * @param array Input array
	 * @param start Start index
	 * @param end End index
	 * @return Returns new pivot position
	 */
	int divide (float* array, int start, int end);

	/**
	 * @brief Sorting function that uses QuickSort algorythm
	 * @param array Input array
	 * @param start Start index
	 * @param end End index
	 */
	void quicksort (float* array, int start, int end);

	/**
	 * @brief Median filter calculation of next value
	 * @param value Next value to do calculation with
	 * @return Returns calculated median
	 */
	float medianFilter (float value);

public:
	/**
	 * @brief Creates a new filter class
	 * @param type Filter type from FilterType_t
	 * @param order Filter order
	 */
	FilterClass (FilterType_t type, uint8_t order);

	/**
	 * @brief Adds a new weighting value. It is pushed on the array so latest value will be used for older data
	 * @param coeff Next weighting coefficient
	 * @return Sum of all weighting values
	 */
	float addWeigth (float coeff);

	/**
	 * @brief Pushes a new value for calculation. Until the buffer is filled up to filter order, only first valid values are used in calculation
	 * @param value Next value
	 * @return Weighted average value
	 */
	float addValue (float value);

	/**
	 * @brief Resets state of the filter to an initial value
	 */
	void clear ();

	/**
	 * @brief Frees up dynamic memory
	 */
	~FilterClass ();
};

#endif

