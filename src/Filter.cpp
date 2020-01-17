// 
// 
// 

#include "Filter.h"
#include "debug.h"

//#ifdef DEBUG
//#define DBG_OUTPUT_PORT Serial
//#define DEBUGLOG(...) DBG_OUTPUT_PORT.printf(__VA_ARGS__)
//#else
//#define DEBUGLOG(...)
//#endif

float FilterClass::addValue(float value)
{
	switch (_filterType) {
		case AVERAGE_FILTER:
			return aveFilter(value);
			break;
		case MEDIAN_FILTER:
			return medianFilter(value);
			break;
		default:
			return value;
	}
}

float FilterClass::addWeigth (float coeff) {
	float sumWeight = 0;

	for (int i = _order - 1; i > 0 ; i--) {
		_weightValues[i] = _weightValues[i - 1];
	}
	_weightValues[0] = coeff;

	for (int i = 0; i < _order; i++) {
		sumWeight += _weightValues[i];
	}
	
	for (int i = 0; i < _order; i++) {
		DEBUG_VERBOSE (" %f", _weightValues[i]);
	}
	DEBUG_VERBOSE ("SumWeight: %f", sumWeight);

	return sumWeight;
}


float FilterClass::aveFilter(float value)
{
	float sumValue = 0;
	float sumWeight = 0;
	float procValue;
	int left, right;

	for (int i = 0; i < _order - 1; i++) {
		_rawValues[i] = _rawValues[i + 1];
	}
	_rawValues[_order - 1] = value;
	
	DEBUG_VERBOSE ("Value: %f\n", value);

	DEBUG_VERBOSE ("Raw values:");
	for (int i = 0; i < _order; i++) {
		DEBUG_VERBOSE (" %f", _rawValues[i]);
	}

	DEBUG_VERBOSE ("Coeffs:");
	for (int i = 0; i < _order; i++) {
		DEBUG_VERBOSE (" %f", _weightValues[i]);
	}

	if (_index < _order) {
		_index++;
		left = _order - _index;
		right = _order - 1;
	}
	else {
		left = 0;
		right = _order - 1;
	}
	DEBUG_VERBOSE ("Index: %d , left: %d , right: %d\n", _index, left, right);

	for (int i = left; i <= right; i++) {
		sumValue += _rawValues[i] * _weightValues[i];
		sumWeight += _weightValues[i];
		//DBG_OUTPUT_PORT.printf("Raw value %d: %f\n", (i + 1), _rawValues[_order - (i+1)]);
	}
	DEBUG_VERBOSE ("Sum: %f", sumValue);
	DEBUG_VERBOSE (" SumWeight: %f\n", sumWeight);

	procValue = sumValue / sumWeight;

	//dtostrf(procValue, 6, 4, strValue);
	DEBUG_VERBOSE ("Average: %f\n", procValue);

	return procValue;
}

// Función para dividir el array y hacer los intercambios
int FilterClass::divide(float *array, int start, int end) {
	int left;
	int right;
	float pivot;
	float temp;

	pivot = array[start];
	left = start;
	right = end;

	// Mientras no se cruzen los índices
	while (left < right) {
		while (array[right] > pivot) {
			right--;
		}

		while ((left < right) && (array[left] <= pivot)) {
			left++;
		}

		// Si todavía no se cruzan los indices seguimos intercambiando
		if (left < right) {
			temp = array[left];
			array[left] = array[right];
			array[right] = temp;
		}
	}

	// Los índices ya se han cruzado, ponemos el pivot en el lugar que le corresponde
	temp = array[right];
	array[right] = array[start];
	array[start] = temp;

	// La nueva posición del pivot
	return right;
}

void FilterClass::clear () {
	for (int i = 0; i < _order; i++) {
		_rawValues[i] = 0;
		_orderedValues[i] = 0;
		_weightValues[i] = 1;
	}
	_index = 0;
}

FilterClass::~FilterClass () {
	free (_rawValues);
	free (_orderedValues);
	free (_weightValues);
}

// Función recursiva para hacer el ordenamiento
void FilterClass::quicksort(float *array, int start, int end)
{
	float pivot;

	if (start < end) {
		pivot = divide(array, start, end);

		// Ordeno la lista de los menores
		quicksort(array, start, pivot - 1);

		// Ordeno la lista de los mayores
		quicksort(array, pivot + 1, end);
	}
}

float FilterClass::medianFilter(float value)
{
	float procValue;
	char strValue[8];
	int medianIdx;
	int left, right, tempidx;
	bool even;
	
	if (_index < _order) {
		_index++;
		left = _order - _index;
		right = _order - 1;
		even = ((right - left) % 2) == 1;
		DEBUG_VERBOSE ("%d: ", (right - left) % 2);
		if (even) {
			tempidx = (right - left - 1) / 2;
			DEBUG_VERBOSE ("even\n");
		}
		else {
			tempidx = (right - left) / 2;
			DEBUG_VERBOSE ("odd\n");
		}
		medianIdx = right - _index + 1 + tempidx;
	}
	else {
		left = 0;
		right = _order - 1;
		even = (_order % 2) == 0;
		if (even)
			tempidx = (right - 1) / 2;
		else
			tempidx = right / 2;
		medianIdx = right - _index + 1 + tempidx;
	}
	DEBUG_VERBOSE ("Index: %d , left: %d , right: %d , even: %s , tempidx: %d , medianidx: %d\n", _index, left, right, (even ? "even" : "odd"), tempidx, medianIdx);

	// Shift raw values
	for (int i = 0; i < _order - 1; i++) {
		_rawValues[i] = _rawValues[i + 1];
	}
	// Add new raw value
	_rawValues[_order - 1] = value;

	DEBUG_VERBOSE ("Raw values:");
	for (int i = 0; i < _order; i++) {
		//dtostrf(_rawValues[i], 6, 4, strValue);
		DEBUG_VERBOSE (" %f", _rawValues[i]);
	}

	// copy to array before ordering
	for (int i = 0; i < _order; i++) {
		_orderedValues[i] = _rawValues[i];
	}

	// order values
	quicksort(_orderedValues, left, right);
	
	DEBUG_VERBOSE ("Ordered values:");
	for (int i = 0; i < _order; i++) {
		//dtostrf(_orderedValues[i], 6, 4, strValue);
		DEBUG_VERBOSE (" %f", _orderedValues[i]);
	}

	// select median value
	if (!even) {
		procValue = _orderedValues[medianIdx];
	}
	else { // there is no center value
		procValue = (_orderedValues[medianIdx] + _orderedValues[medianIdx+1]) / 2.0F;
	}

	//dtostrf(procValue, 6, 4, strValue);
	DEBUG_VERBOSE ("Median: %f\n", procValue);
	return procValue; // return mid value
}

FilterClass::FilterClass(FilterType_t type, uint8_t order)
{
	_filterType = type;

	if (order < MAX_ORDER)
		if (order > 1)
			_order = order;
		else
			_order = MIN_ORDER;
	else
		_order = MAX_ORDER;

	_rawValues = (float *)malloc(_order * sizeof(float));
	for (int i = 0; i < _order; i++) {
		_rawValues[i] = 0;
	}

	_orderedValues = (float *)malloc(_order * sizeof(float));
	for (int i = 0; i < _order; i++) {
		_orderedValues[i] = 0;
	}
	
	_weightValues = (float *)malloc(_order * sizeof(float));
	for (int i = 0; i < _order; i++) {
		_weightValues[i] = 1;
	}

}


//FilterClass Filter;