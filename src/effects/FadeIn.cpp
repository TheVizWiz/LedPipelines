#include "effects/FadeIn.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


FadeIn::FadeIn(LedPipelineStage* stage, unsigned long runtimeMs, SmoothingFunction smoothingFunction,
			   BlendingMode blendingMode) :
	WrapperEffect(stage), TimedEffect(runtimeMs), smoothingFunction(smoothingFunction) {
	this->blendingMode = blendingMode;
}

void FadeIn::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	// Render the wrapped effect first, then scale its opacity by the fade ramp.
	this->stage->calculate(startIndex, tempData);

	// elapsedMs() is 0 during the lead-in delay, so a delayed FadeIn keeps its inner invisible until the delay elapses,
	// then ramps in.
	unsigned long timeFadingMs = elapsedMs();

	if (timeFadingMs >= runtimeMs) {
		// Ramp complete: full opacity, so leave the inner effect's opacity untouched (multiplier == 1). We do NOT go
		// DONE here - FadeIn is a pass-through once faded in, and terminating would hide the wrapped content.
		elapsedPercentage = 1;
	} else {
		elapsedPercentage = (float)timeFadingMs / (float)runtimeMs;
		float opacityMultiplier = smoothingFunction(timeFadingMs, 0, runtimeMs, 0, UINT8_MAX);
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.opacity[i] = (tempData.opacity[i] * opacityMultiplier) / 255;
		}
	}

	// Defer termination to the inner effect: FadeIn is DONE only when the wrapped effect finishes. We do NOT copy the
	// inner's state wholesale - an inner that reports NOT_STARTED every frame (e.g. Solid, whose calculate() never sets
	// RUNNING) would otherwise keep re-initializing our fade timer, freezing the ramp.
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void FadeIn::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
}


RandomFadeIn::RandomFadeIn(LedPipelineStage* stage, const unsigned long minRuntimeMs, const unsigned long maxRuntimeMs,
						   SamplingFunction samplingFunction, SmoothingFunction smoothingFunction,
						   BlendingMode blendingMode) :
	WrapperEffect(stage), RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction),
	smoothingFunction(smoothingFunction) {
	this->blendingMode = blendingMode;
}

void RandomFadeIn::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->sampleRuntime();
		LPLogger::log(String("running random fade in effect for ") + this->runtimeMs + " ms");
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->stage->calculate(startIndex, tempData);

	unsigned long timeFadingMs = elapsedMs();

	if (timeFadingMs >= runtimeMs) {
		LPLogger::log("done running random fade in effect.");
		elapsedPercentage = 1;
	} else {
		elapsedPercentage = static_cast<float>(timeFadingMs) / static_cast<float>(runtimeMs);
		float opacityMultiplier = smoothingFunction(timeFadingMs, 0, runtimeMs, 0, UINT8_MAX);
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.opacity[i] = (tempData.opacity[i] * opacityMultiplier) / 255;
		}
	}

	// Defer termination to the inner effect (see FadeIn::calculate for why we don't copy the inner state wholesale).
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void RandomFadeIn::reset() {
	WrapperEffect::reset();
	RandomTimedEffect::resetTimer();
}
