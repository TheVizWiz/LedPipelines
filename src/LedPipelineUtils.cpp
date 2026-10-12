#include "BaseLedPipeline.h"
#include "LedOutput.h"

namespace ledpipelines {
	uint64_t minMicrosBetweenUpdates = 0;

	void setMaxRefreshRate(float refreshesPerSecond) {
		minMicrosBetweenUpdates = (uint64_t)(1000000 / refreshesPerSecond);
	}

	static LedOutput* g_output = nullptr;

	void setOutput(LedOutput* output) {
		g_output = output;
	}

	LedOutput* getOutput() {
		return g_output;
	}
}
