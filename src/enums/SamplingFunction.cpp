#include "enums/SamplingFunction.h"

#include <cstdlib>

using namespace ledpipelines;

SamplingFunction::SamplingFunction(SamplingFunction_ preset) {
#ifdef LEDPIPELINES_SERIALIZATION
	presetValue = preset;
	builtFromPreset = true;
#endif
	switch (preset) {
	case UNIFORM:
		warp = [](float x) { return x; };
		break;
	// Flat around x=0.5, so samples cluster in the middle.
	case CENTERED:
		warp = [](float x) {
			float t = 2 * x - 1;
			return 0.5f * (1 + t * t * t);
		};
		break;
	// Smoothstep: flat at x=0 and x=1, so samples pile up at both edges.
	case EDGES:
		warp = [](float x) { return x * x * (3 - 2 * x); };
		break;
	}
}

float SamplingFunction::operator()(float min, float max) const {
	float originalRandomValue = (float)rand() / (float)RAND_MAX;
	float warped = warp(originalRandomValue);
	return (max - min) * warped + min;
}
