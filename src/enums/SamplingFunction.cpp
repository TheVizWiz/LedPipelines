#include "enums/SamplingFunction.h"
#include "LedPipelinesLogger.h"

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
	case CENTERED: randomFunctionOutput = tanh(5 * (originalRandomValue - 0.5)) / tanh_range; break;
	}

	return (max - min) * randomFunctionOutput + min;
}
