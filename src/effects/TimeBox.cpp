#include "effects/TimeBox.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

TimeBox::TimeBox(LedPipelineStage* stage, unsigned long runtimeMs) : WrapperEffect(stage), TimedEffect(runtimeMs) {}


void TimeBox::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		startTimeMs = millis();
	}

	// elapsedMs() holds at 0 during the lead-in delay, so a delayed TimeBox shows its inner for the delay before its
	// own runtime clock starts counting.
	unsigned long elapsedTime = elapsedMs();


	this->elapsedPercentage = (float)elapsedTime / (float)runtimeMs;
	this->stage->calculate(startIndex, tempData);
	// if the internal stage is done, we set it to done.
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}

	if (elapsedTime >= runtimeMs) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}
}


void TimeBox::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
}

RandomTimeBox::RandomTimeBox(
	LedPipelineStage* stage,
	unsigned long minRuntimeMs,
	unsigned long maxRuntimeMs,
	SamplingFunction samplingFunction
)
	: WrapperEffect(stage), RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction) {}


void RandomTimeBox::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		this->sampleRuntime();
		startTimeMs = millis();
	}

	unsigned long elapsedTime = elapsedMs();


	this->elapsedPercentage = (float)elapsedTime / (float)runtimeMs;
	this->stage->calculate(startIndex, tempData);
	// if the internal stage is done, we set it to done.
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}

	if (elapsedTime >= runtimeMs) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}
}


void RandomTimeBox::reset() {
	WrapperEffect::reset();
	RandomTimedEffect::resetTimer();
}
