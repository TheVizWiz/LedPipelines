// Distribution tests for SamplingFunction (include/enums/SamplingFunction.h). Part of the test_base_utils suite: no
// main() here - BaseLedPipelineTest.cpp owns the suite's main(), and PlatformIO links every .cpp in the folder into one
// binary.
//
// A SamplingFunction warps a uniform draw from [0, 1] into a skewed distribution over [min, max]. Its contract is the
// SHAPE of that distribution, not any single point value, so we test it statistically: draw many samples from a seeded
// RNG, bin them into a histogram, and assert the histogram has the expected shape. This is what catches shape bugs -
// an earlier CENTERED implementation pinned the right endpoints yet clustered at the EDGES instead of the center, which
// no endpoint/monotonicity check would have flagged.

#include <gtest/gtest.h>

#include <cmath>    // std::pow (custom-warp test)
#include <cstdlib>  // srand
#include <vector>

#include "LedPipelines.h"

using namespace ledpipelines;

namespace {
	constexpr int kSamples = 200000;
	constexpr int kBins = 10;

	// Draw kSamples from `fn` over [0, 1] and return the fraction of samples landing in each of kBins equal-width bins.
	// Seeds rand() first so the run is deterministic. Samples are expected within [0, 1]; anything outside is clamped
	// into the edge bins so an out-of-range regression still shows up as a lopsided histogram rather than a crash.
	std::vector<double> histogram(const SamplingFunction &fn) {
		srand(12345);
		std::vector<int> counts(kBins, 0);
		for (int i = 0; i < kSamples; i++) {
			float sample = fn(0.0f, 1.0f);
			int bin = static_cast<int>(sample * kBins);
			if (bin < 0) {
				bin = 0;
			}
			if (bin >= kBins) {
				bin = kBins - 1;
			}
			counts[bin]++;
		}

		std::vector<double> fractions(kBins);
		for (int i = 0; i < kBins; i++) {
			fractions[i] = static_cast<double>(counts[i]) / kSamples;
		}
		return fractions;
	}
}  // namespace

// UNIFORM is the identity warp f(x) = x, so a uniform draw stays uniform: every bin should hold roughly 1/kBins of the
// samples. With 200k samples the per-bin fraction sits tightly around 0.10; a 0.02 tolerance is generous slack for RNG
// noise while still failing if any bin is systematically over/under-represented.
TEST(SamplingFunctionTest, UniformIsFlat) {
	auto bins = histogram(SamplingFunction::UNIFORM);

	for (int i = 0; i < kBins; i++) {
		EXPECT_NEAR(bins[i], 1.0 / kBins, 0.02) << "UNIFORM bin " << i << " should be ~1/kBins (flat)";
	}
}

// CENTERED biases samples toward the middle of the range: the density peaks at 0.5 and thins toward the edges. We
// assert that shape directly - the two center bins each dominate the two edge bins - which is exactly the property the
// old edge-clustering implementation violated.
TEST(SamplingFunctionTest, CenteredClustersInTheMiddle) {
	auto bins = histogram(SamplingFunction::CENTERED);

	double centerLow = bins[kBins / 2 - 1];  // bin [0.4, 0.5)
	double centerHigh = bins[kBins / 2];     // bin [0.5, 0.6)
	double edgeLow = bins[0];                // bin [0.0, 0.1)
	double edgeHigh = bins[kBins - 1];       // bin [0.9, 1.0)

	// The center must be denser than the edges - the defining property of CENTERED. A wide margin keeps this robust to
	// RNG noise while still failing hard on the inverted (edge-clustering) shape.
	EXPECT_GT(centerLow, edgeLow * 3) << "center should be far denser than the low edge";
	EXPECT_GT(centerHigh, edgeHigh * 3) << "center should be far denser than the high edge";

	// Symmetry about 0.5: the cubic warp is odd about the midpoint, so mirror-image bins carry near-equal mass.
	EXPECT_NEAR(centerLow, centerHigh, 0.02) << "CENTERED should be symmetric about 0.5 at the center";
	EXPECT_NEAR(edgeLow, edgeHigh, 0.02) << "CENTERED should be symmetric about 0.5 at the edges";

	// Density should fall off monotonically from the center out to each edge - no secondary peaks or edge spikes.
	for (int i = 0; i < kBins / 2 - 1; i++) {
		EXPECT_LE(bins[i], bins[i + 1]) << "left half should rise toward the center (bin " << i << ")";
	}
	for (int i = kBins / 2; i < kBins - 1; i++) {
		EXPECT_GE(bins[i], bins[i + 1]) << "right half should fall away from the center (bin " << i << ")";
	}
}

// EDGES is the mirror image of CENTERED: it biases samples toward both ends of the range, with density peaking at 0 and
// 1 and thinning in the middle. This is the behavior the old (broken) CENTERED accidentally produced; here it is the
// intended contract, so we assert it directly - the edge bins dominate the center bins.
TEST(SamplingFunctionTest, EdgesClustersAtTheEnds) {
	auto bins = histogram(SamplingFunction::EDGES);

	double centerLow = bins[kBins / 2 - 1];  // bin [0.4, 0.5)
	double centerHigh = bins[kBins / 2];     // bin [0.5, 0.6)
	double edgeLow = bins[0];                // bin [0.0, 0.1)
	double edgeHigh = bins[kBins - 1];       // bin [0.9, 1.0)

	// The edges must be denser than the center - the defining property of EDGES, and the exact inverse of CENTERED.
	EXPECT_GT(edgeLow, centerLow * 2) << "low edge should be far denser than the center";
	EXPECT_GT(edgeHigh, centerHigh * 2) << "high edge should be far denser than the center";

	// Symmetry about 0.5: smoothstep is odd about the midpoint, so mirror-image bins carry near-equal mass.
	EXPECT_NEAR(edgeLow, edgeHigh, 0.02) << "EDGES should be symmetric about 0.5 at the edges";
	EXPECT_NEAR(centerLow, centerHigh, 0.02) << "EDGES should be symmetric about 0.5 at the center";

	// Density should fall off monotonically from each edge in toward the center - no dip-then-rise or center spike.
	for (int i = 0; i < kBins / 2 - 1; i++) {
		EXPECT_GE(bins[i], bins[i + 1]) << "left half should fall away from the low edge (bin " << i << ")";
	}
	for (int i = kBins / 2; i < kBins - 1; i++) {
		EXPECT_LE(bins[i], bins[i + 1]) << "right half should rise toward the high edge (bin " << i << ")";
	}
}

// A user can supply their own warp instead of a preset. f(x) = x^2 biases toward the low end (min): its slope is zero
// at x=0 (flat -> dense) and steepest at x=1, so the low bins dominate the high bins and the distribution is NOT
// symmetric. This exercises the custom-warp constructor and confirms an arbitrary callable reshapes the distribution.
TEST(SamplingFunctionTest, CustomWarpBiasesTowardMin) {
	SamplingFunction squared([](float x) { return x * x; });
	auto bins = histogram(squared);

	EXPECT_GT(bins[0], bins[kBins - 1] * 5) << "x^2 should pack the low end far denser than the high end";

	// Strictly decreasing from the low edge outward - a monotone one-sided skew, no interior peak.
	for (int i = 0; i < kBins - 1; i++) {
		EXPECT_GE(bins[i], bins[i + 1]) << "x^2 density should fall monotonically from min to max (bin " << i << ")";
	}
}

// A custom warp can capture parameters (the reason for std::function over a raw function pointer). Here the exponent k
// is captured, so the same lambda shape yields different skews. This is the parameterized-warp use case that a bare
// function pointer could not express.
TEST(SamplingFunctionTest, CustomWarpCanCaptureParameters) {
	float k = 3.0f;
	SamplingFunction powered([k](float x) { return std::pow(x, k); });
	auto bins = histogram(powered);

	// A steeper exponent packs even harder toward min than x^2 did.
	EXPECT_GT(bins[0], bins[kBins - 1] * 10) << "x^3 should pack the low end even harder than x^2";
}
