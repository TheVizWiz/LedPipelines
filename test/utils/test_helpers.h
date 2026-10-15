#pragma once

#include <gtest/gtest.h>

#include "LedPipelines.h"

// Shared test helpers. Everything here must be header-inline, since every suite's translation unit includes it.

namespace ledpipelines {}

using namespace ledpipelines;
using namespace ledpipelines::effects;

namespace test_helpers {
	constexpr int kLedCount = 10;

	// A single strip of kLedCount pixels written into `pixels`, which a test can inspect.
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

	inline TestOutput gOutput;

	// Re-registering the same output each time keeps topology stable across tests.
	inline void setUpLeds() {
		setOutput(&gOutput);
		TemporaryLedData::initialize();
	}

	// A no-op leaf effect counting live and total instances, to observe whether builders rebuild a fresh inner on each
	// build() and whether wrappers delete their inner.
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

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override {
			std::string result = "{";
			result += jsonHeader("SpyEffect");
			result += "}";
			return result;
		}
#endif

		struct Builder : LedPipelineStage::Builder<SpyEffect, Builder> {
			SpyEffect *create() override { return new SpyEffect(); }
		};
	};
}

using test_helpers::setUpLeds;
using test_helpers::SpyEffect;
