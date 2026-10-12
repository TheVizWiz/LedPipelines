// A SamplingFunction's contract is the SHAPE of its distribution, not any single point value, so these tests bin many
// seeded samples into a histogram and assert on its shape.

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <vector>

#include "LedPipelines.h"

using namespace ledpipelines;

namespace {
	constexpr int kSamples = 200000;
	constexpr int kBins = 10;

	// Returns the fraction of kSamples (seeded, deterministic) landing in each of kBins equal-width bins over [0, 1].
	// Out-of-range samples are clamped into the edge bins so a regression shows up as a lopsided histogram, not a crash.
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
}

TEST(SamplingFunctionTest, UniformIsFlat) {
	auto bins = histogram(SamplingFunction::UNIFORM);

	for (int i = 0; i < kBins; i++) {
		EXPECT_NEAR(bins[i], 1.0 / kBins, 0.02) << "UNIFORM bin " << i << " should be ~1/kBins (flat)";
	}
}

TEST(SamplingFunctionTest, CenteredClustersInTheMiddle) {
	auto bins = histogram(SamplingFunction::CENTERED);

	double centerLow = bins[kBins / 2 - 1];
	double centerHigh = bins[kBins / 2];
	double edgeLow = bins[0];
	double edgeHigh = bins[kBins - 1];

	EXPECT_GT(centerLow, edgeLow * 3) << "center should be far denser than the low edge";
	EXPECT_GT(centerHigh, edgeHigh * 3) << "center should be far denser than the high edge";

	EXPECT_NEAR(centerLow, centerHigh, 0.02) << "CENTERED should be symmetric about 0.5 at the center";
	EXPECT_NEAR(edgeLow, edgeHigh, 0.02) << "CENTERED should be symmetric about 0.5 at the edges";

	for (int i = 0; i < kBins / 2 - 1; i++) {
		EXPECT_LE(bins[i], bins[i + 1]) << "left half should rise toward the center (bin " << i << ")";
	}
	for (int i = kBins / 2; i < kBins - 1; i++) {
		EXPECT_GE(bins[i], bins[i + 1]) << "right half should fall away from the center (bin " << i << ")";
	}
}

TEST(SamplingFunctionTest, EdgesClustersAtTheEnds) {
	auto bins = histogram(SamplingFunction::EDGES);

	double centerLow = bins[kBins / 2 - 1];
	double centerHigh = bins[kBins / 2];
	double edgeLow = bins[0];
	double edgeHigh = bins[kBins - 1];

	EXPECT_GT(edgeLow, centerLow * 2) << "low edge should be far denser than the center";
	EXPECT_GT(edgeHigh, centerHigh * 2) << "high edge should be far denser than the center";

	EXPECT_NEAR(edgeLow, edgeHigh, 0.02) << "EDGES should be symmetric about 0.5 at the edges";
	EXPECT_NEAR(centerLow, centerHigh, 0.02) << "EDGES should be symmetric about 0.5 at the center";

	for (int i = 0; i < kBins / 2 - 1; i++) {
		EXPECT_GE(bins[i], bins[i + 1]) << "left half should fall away from the low edge (bin " << i << ")";
	}
	for (int i = kBins / 2; i < kBins - 1; i++) {
		EXPECT_LE(bins[i], bins[i + 1]) << "right half should rise toward the high edge (bin " << i << ")";
	}
}

TEST(SamplingFunctionTest, CustomWarpBiasesTowardMin) {
	SamplingFunction squared([](float x) { return x * x; });
	auto bins = histogram(squared);

	EXPECT_GT(bins[0], bins[kBins - 1] * 5) << "x^2 should pack the low end far denser than the high end";

	for (int i = 0; i < kBins - 1; i++) {
		EXPECT_GE(bins[i], bins[i + 1]) << "x^2 density should fall monotonically from min to max (bin " << i << ")";
	}
}

TEST(SamplingFunctionTest, CustomWarpCanCaptureParameters) {
	float k = 3.0f;
	SamplingFunction powered([k](float x) { return std::pow(x, k); });
	auto bins = histogram(powered);

	EXPECT_GT(bins[0], bins[kBins - 1] * 10) << "x^3 should pack the low end even harder than x^2";
}
