#include "enums/SamplingFunction.h"

#include <cmath>   // tanh
#include <cstdlib>  // rand, RAND_MAX

using namespace ledpipelines;

SamplingFunction& SamplingFunction::operator=(SamplingFunction& other) {
	this->value = other.value;
	return *this;
}

float SamplingFunction::operator()(float min, float max) const {
	float originalRandomValue = (float)rand() / (float)RAND_MAX;

	float randomFunctionOutput;

	switch (value) {
	case UNIFORM: randomFunctionOutput = originalRandomValue; break;
	// Normalize the tanh output back into [0, 1]: tanh(5*(x-0.5)) spans [tanh_min, tanh_max] as x goes 0->1, so shift
	// by tanh_min before dividing by the range. Without the -tanh_min shift this yielded [-0.5, 0.5], which mapped
	// samples to [min - range/2, min + range/2] - producing near-zero and negative durations (fades that pop instead
	// of ramp). x=0 -> 0, x=0.5 -> 0.5, x=1 -> 1, biased toward the center.
	case CENTERED:
		randomFunctionOutput = (tanh(5 * (originalRandomValue - 0.5)) - tanh_min) / tanh_range;
		break;
	}

	return (max - min) * randomFunctionOutput + min;
}
