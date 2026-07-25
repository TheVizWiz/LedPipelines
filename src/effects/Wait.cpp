#include <utility>

#include "effects/Wait.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


Wait::Wait(unsigned long runtimeMs, BlendingMode blendingMode)
	: LedPipelineStage(blendingMode), TimedEffect(runtimeMs) {}


void Wait::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		this->startTimeMs = millis();
	}

	unsigned long totalTimeWaited = elapsedMs();

	if (totalTimeWaited >= this->runtimeMs) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	this->elapsedPercentage = (float)totalTimeWaited / (float)this->runtimeMs;
	this->state = LedPipelineRunningState::RUNNING;
}

void Wait::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
}


RandomWait::RandomWait(
	unsigned long minRuntimeMs,
	unsigned long maxRuntimeMs,
	SamplingFunction samplingFunction,
	BlendingMode blendingMode
)
	: LedPipelineStage(blendingMode), RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction) {}

void RandomWait::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) {
		return;
	}

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		this->startTimeMs = millis();
		this->sampleRuntime();
	}

	unsigned long totalTimeWaited = elapsedMs();

	if (totalTimeWaited >= this->runtimeMs) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	this->elapsedPercentage = (float)totalTimeWaited / (float)this->runtimeMs;
	this->state = LedPipelineRunningState::RUNNING;
}

void RandomWait::reset() {
	LedPipelineStage::reset();
	RandomTimedEffect::resetTimer();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Wait::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Wait");
	result += jsonField("runtimeMs", (int)runtimeMs);
	result += jsonField("delayMs", (int)delayMs);
	result += jsonField("terminateOnComplete", terminateOnComplete);
	result += "}";
	return result;
}

std::string RandomWait::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("RandomWait");
	result += jsonField("minRuntimeMs", (int)minRuntimeMs);
	result += jsonField("maxRuntimeMs", (int)maxRuntimeMs);
	result += jsonField("samplingFunction", samplingFunction);
	result += jsonField("delayMs", (int)delayMs);
	result += jsonField("terminateOnComplete", terminateOnComplete);
	result += "}";
	return result;
}
#endif
