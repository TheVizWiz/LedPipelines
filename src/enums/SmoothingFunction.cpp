#include "enums/SmoothingFunction.h"
#include <cmath>

using namespace ledpipelines;

// M_PI is not standard C++ - it's a POSIX/glibc extension. macOS and Linux expose it from <cmath>, but MinGW (Windows)
// only defines it when _USE_MATH_DEFINES is set before <cmath> is first included, which is fragile to rely on. Define
// our own pi so the smoothing math is portable everywhere without depending on that.
namespace {
	constexpr float kPi = 3.14159265358979323846f;
}


float SmoothingFunction::operator()(float amount, float oldMin, float oldMax, float newMin, float newMax) const {
	/**
	 * min and max aren't guaranteed to actually be min and max. e.g. it could be min < amount < max, OR
	 * max < amount < min. This logic clamps to the range (min, max) regardless of which one is smaller and which one
	 * is larger.
	 */
	if (oldMin < oldMax) {
		if (amount < oldMin) {
			amount = oldMin;
		} else if (amount > oldMax) {
			amount = oldMax;
		}
	} else {
		if (amount > oldMin) {
			amount = oldMin;
		} else if (amount < oldMax) {
			amount = oldMax;
		}
	}

	float oldRange = (oldMax - oldMin);
	float amountInRange = (amount - oldMin);
	float percentage = amountInRange / oldRange;

	switch (this->value) {
	case SMOOTH_LINEAR:
		// f(x) = 0.5 * cos(pi * x + pi) + 0.5
		percentage = 0.5f * cosf(kPi * percentage + kPi) + 0.5f;
		break;
	case LINEAR:
		// f(x) = x, so we don't do anything.
		break;
	case SINE:
		// f(x) = sin(pi * x / 2)
		percentage = sinf(kPi * percentage / 2);
		break;
	case QUADRATIC:
		// f(x) = x^2
		percentage = pow(percentage, 2);
		break;
	case INVERSE_QUADRATIC:
		// f(x) = 1 - (x - 1)^2
		percentage = 1 - pow(percentage - 1, 2);
		break;
	}

	return (newMin + (newMax - newMin) * percentage);
}

SmoothingFunction& SmoothingFunction::operator=(const SmoothingFunction& other) {
	this->value = other.value;
	return *this;
}
