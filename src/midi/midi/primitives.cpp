#include "primitives.h"

namespace midi {
	Duration operator +(const Duration& dur1, const Duration& dur2) {
		return Duration(value(dur1) + value(dur2));
	};

	Duration operator -(const Duration& dur1, const Duration& dur2) {
		return Duration(value(dur1) - value(dur2));
	};

	Duration operator -(const Time& time1, const Time& time2) {
		return Duration(value(time1) - value(time2));
	};

	Time operator +(const Time& time1, const Duration& dur1){
		return Time(value(time1) + value(dur1));
	};

	Time operator +(const Duration& dur1, const Time& time1) {
		return Time(value(dur1) + value(time1));
	};


	Time& operator +=(Time& time1, const Duration& dur1) {
		value(time1) += value(dur1);
		return time1;
	};
	Duration& operator +=(Duration& dur1, const Duration& dur2) {
		value(dur1) += value(dur2);
		return dur1;
	};
	Duration& operator -=(Duration& dur1, const Duration& dur2) {
		value(dur1) -= value(dur2);
		return dur1;
	};
}