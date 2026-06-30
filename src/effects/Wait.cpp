#include <utility>

#include "effects/Wait.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


Wait::Wait(unsigned long runtimeMs) : TimedEffect(runtimeMs) {}


void Wait::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		this->startTimeMs = millis();
	}

	unsigned long totalTimeWaited = millis() - this->startTimeMs;

	if (totalTimeWaited / 1000.0 >= this->runtimeMs) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	this->elapsedPercentage = ((float)totalTimeWaited / 1000.0f) / this->runtimeMs; // convert ms to seconds
	this->state = LedPipelineRunningState::RUNNING;
}

void Wait::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
}


RandomWaitEffect::RandomWaitEffect(unsigned long minRuntimeMs, unsigned long maxRuntimeMs,
								   SamplingFunction samplingFunction) :
	RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction) {}

void RandomWaitEffect::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) {
		return;
	}

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		this->startTimeMs = millis();
		this->sampleRuntime();
		LPLogger::log(String("running random wait effect for ") + this->runtimeMs + " seconds");
	}

	unsigned long totalTimeWaited = millis() - this->startTimeMs;

	if (totalTimeWaited / 1000.0 >= this->runtimeMs) {
		LPLogger::log("done running random wait effect.");
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	this->elapsedPercentage = ((float)totalTimeWaited / 1000.0f) / this->runtimeMs; // convert ms to seconds
	this->state = LedPipelineRunningState::RUNNING;
}

void RandomWaitEffect::reset() {
	LedPipelineStage::reset();
	RandomTimedEffect::resetTimer();
}
