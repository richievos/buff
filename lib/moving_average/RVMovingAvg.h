// Based on ZzzMovingAvg, but without the default filling
/***************************************************************
  RVMovingAverage.h
***************************************************************/
#ifndef RV_MOVING_AVG_H
#define RV_MOVING_AVG_H

/**
 * Compute Simple moving average (boxcar filter)
 * https://en.wikipedia.org/wiki/Moving_average
 *
 * N     is the number of values to smooth the data (default=4). Could be a power of 2 (2,4,8,16,32...) to optimize computation at compile time.
 * T     is the type of data (default=int). Could be uint8_t, int8_t, uint16_t, int, float...
 * TSUM  is the type of the sum (default=long). Should be able to contain N*data without overflowing.
 *       if N is 8 and values could go up to 100, then T could be uint8_t (max=255) and TSUM should at least be uint16_t (max=65535).
 */
template <size_t N=4,typename T=int, typename TSUM=long> class RVMovingAvg {
	private:
		T _data[N];
		T _result;
		TSUM _sum;
		size_t _current;
		size_t _currentSize;
		bool _first;

	public:
		/** Next value will be filled */
		void reset() {
			_first=true;
			_current=N-1;
			_sum=0;
			_result=0;
			_currentSize=0;
		}

		/** Fill all values with given value and set result to value */
		void fill(T value) {
			for (size_t i=0; i<N; i++) {
				_data[i]=value;
			}
			_sum=value*N;
			_result=value;
			_current=N-1;
			_currentSize=N;
			_first=false;
		}

		/** Constructor first value will be filled */
		RVMovingAvg() {
			reset();
		}

		/** Constructor filled with given initial value */
		RVMovingAvg(T initialValue) {
			fill(initialValue);
		}

		/**
		 * Add a new value and compute the new moving average.
		 * @return the new moving average
		 */
		T add(T input) {
            //Compute new index
            _current++;
            if (_current>=N) {
                _current=0;
			}
            _sum-=_data[_current];
            _sum+=input;
            _data[_current]=input;
            if (_currentSize < N) {
                _currentSize++;
            }
			_result=_sum/_currentSize;
			return _result;
		}

		/** Get current moving average */
		T get() {
			return _result; 
		}

		/**
		 * Get previous value
		 * getLast(), or getLast(0) return the last added value
		 * getLast(1) return value added before the last one
		 * ...
		 * getLast(N-1) return the last value accessible
		 */
		T getLast(size_t back=0) {
			//ie: (N=4) _current=1 back 0,1,2,3 => returns 1,0,3,2
			if (back>=N) {
				back=N-1;
			}
			if (_current>=back) {
				return _data[_current-back];
			}
			else {
				return _data[N+_current-back];
			}
		}

		/** Get data size */
		size_t size() {
			return _currentSize;
		}
};
#endif

