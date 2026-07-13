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
