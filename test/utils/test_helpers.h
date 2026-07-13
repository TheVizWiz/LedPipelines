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
	constexpr int kLedCount = 10;

	// A minimal LedOutput backing the tests: a single strip of kLedCount pixels into a local buffer. It stands in for a
	// real backend (FastLED, ESPHome) so the core can be exercised natively - initialize() reads its topology and
	// populate() writes pixels into `pixels`, which a test can inspect. clear()/show() are no-ops.
	struct TestOutput : LedOutput {
		RGBA pixels[kLedCount] = {};

		int stripCount() const override { return 1; }
		int stripSize(int) const override { return kLedCount; }
		void clear() override {}
		void show() override {}
		void setPixel(int, int indexInStrip, RGBA color) override {
			if (indexInStrip >= 0 && indexInStrip < kLedCount) pixels[indexInStrip] = color;
		}
	};

	// The single output instance the tests render into.
	inline TestOutput gOutput;

	// Register the test output and initialize LedPipelines' static LED bookkeeping. Re-registering the same output each
	// time keeps topology stable across tests.
	inline void setUpLeds() {
		setOutput(&gOutput);
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
