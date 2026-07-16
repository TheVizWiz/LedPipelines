#include "enums/SamplingFunction.h"

#include <cstdlib>  // rand, RAND_MAX

using namespace ledpipelines;

// Map a preset enum to its warp. The warp reshapes a uniform draw x in [0, 1] into [0, 1]; operator() rescales the
// result into [min, max].
SamplingFunction::SamplingFunction(SamplingFunction_ preset) {
	switch (preset) {
	// Identity: a uniform draw stays uniform.
	case UNIFORM:
		warp = [](float x) { return x; };
		break;
	// Cubic warp centered at 0.5, biasing samples toward the center. Density is high where the warp is flat and low
	// where it is steep: (2x-1)^3 is flat around x=0.5 and steep near the edges, so a uniform draw clusters in the
	// middle. Pins f(0)=0, f(0.5)=0.5, f(1)=1 and is monotonic, so outputs stay within [min, max].
	case CENTERED:
		warp = [](float x) {
			float t = 2 * x - 1;
			return 0.5f * (1 + t * t * t);
		};
		break;
	// Smoothstep warp (3x^2 - 2x^3), the inverse skew of CENTERED: it biases samples toward BOTH edges. Its slope is
	// zero at x=0 and x=1 (flat -> samples pile up at the edges) and steepest at x=0.5 (thins the middle). Pins f(0)=0,
	// f(0.5)=0.5, f(1)=1 and is monotonic, so outputs stay within [min, max].
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
