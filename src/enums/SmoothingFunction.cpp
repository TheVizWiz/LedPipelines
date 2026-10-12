#include "enums/SmoothingFunction.h"
#include <cmath>

using namespace ledpipelines;

// M_PI is non-standard; MinGW only defines it with _USE_MATH_DEFINES set before <cmath>.
namespace {
	constexpr float kPi = 3.14159265358979323846f;
}

float SmoothingFunction::operator()(float amount, float oldMin, float oldMax, float newMin, float newMax) const {
	/** oldMin may be greater than oldMax; clamp to the range regardless of order. */
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
		percentage = 0.5f * cosf(kPi * percentage + kPi) + 0.5f;
		break;
	case LINEAR:
		break;
	case SINE:
		percentage = sinf(kPi * percentage / 2);
		break;
	case QUADRATIC:
		percentage = pow(percentage, 2);
		break;
	case INVERSE_QUADRATIC:
		percentage = 1 - pow(percentage - 1, 2);
		break;
	}

	return (newMin + (newMax - newMin) * percentage);
}

SmoothingFunction& SmoothingFunction::operator=(const SmoothingFunction& other) {
	this->value = other.value;
	return *this;
}
