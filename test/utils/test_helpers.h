#pragma once

#include <gtest/gtest.h>

#include "LedPipelines.h"

// Shared test fixtures and helpers used across the test suites (test_effects, test_base_utils, ...). This is test-only
// support code, not a suite itself - it lives under test/utils/ (no test_ prefix, so PlatformIO does not build it as a
// suite) and each suite includes it via "../utils/test_helpers.h". Everything here is a header-inline definition
// (inline vars, inline functions) so it can be safely included into every suite's separate translation unit without
// violating the one-definition rule.

namespace ledpipelines {}  // ensure namespace exists for the usings below

using namespace ledpipelines;
using namespace ledpipelines::effects;

namespace test_helpers {
	// Backing buffer for a single fake strip. Registered with the stub FastLED controller so TemporaryLedData::size is
	// non-zero once initialize() runs.
	constexpr int kLedCount = 10;
	inline CRGB gLeds[kLedCount];

	// Register one strip and initialize LedPipelines' static LED bookkeeping. Resets the fake controller first so strips
	// don't accumulate across tests.
	inline void setUpLeds() {
		FastLED.reset();
		FastLED.addLeds(gLeds, kLedCount);
		TemporaryLedData::initialize();
	}

	// A leaf effect that tracks how many instances currently exist (liveCount) and how many have ever been constructed
	// (totalCreated). Used to observe, from outside, whether the builder rebuilds a fresh inner effect on each build()
	// and whether wrappers correctly own/delete their inner. calculate() is a no-op.
	struct SpyEffect : LedPipelineStage {
		static inline int liveCount = 0;
		static inline int totalCreated = 0;

		static void resetCounters() {
			liveCount = 0;
			totalCreated = 0;
		}

		SpyEffect() {
			liveCount++;
			totalCreated++;
		}

		~SpyEffect() override { liveCount--; }

		void calculate(float, TemporaryLedData &) override {}

		struct Builder : LedPipelineStage::Builder<SpyEffect, Builder> {
			SpyEffect *build() override { return new SpyEffect(); }
		};
	};
}

using test_helpers::setUpLeds;
using test_helpers::SpyEffect;
