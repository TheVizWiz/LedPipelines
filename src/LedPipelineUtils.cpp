#include "BaseLedPipeline.h"
#include "LedOutput.h"

namespace ledpipelines {
	// Single shared definitions of the library-wide globals declared in BaseLedPipeline.h / LedOutput.h. They live in
	// one translation unit (rather than inline in a header) so every unit sees the SAME instance - otherwise a
	// setOutput() call in the sketch would not be visible to run() compiled in this file.

	uint64_t minMicrosBetweenUpdates = 0;

	void setMaxRefreshRate(float refreshesPerSecond) {
		minMicrosBetweenUpdates = (uint64_t)(1000000 / refreshesPerSecond);
	}

	// The registered render backend. Null until setOutput() is called; core code guards on it.
	static LedOutput* g_output = nullptr;

	void setOutput(LedOutput* output) {
		g_output = output;
	}

	LedOutput* getOutput() {
		return g_output;
	}
} // namespace ledpipelines
