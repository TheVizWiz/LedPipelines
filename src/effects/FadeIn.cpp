#include "effects/FadeIn.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


FadeIn::FadeIn(unsigned long runtimeMs, SmoothingFunction smoothingFunction, BlendingMode blendingMode) :
	LedPipelineStage(blendingMode), TimedEffect(runtimeMs), smoothingFunction(smoothingFunction) {}

void FadeIn::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	unsigned long currentTimeMillis = millis();

	unsigned long timeFadingMs = (currentTimeMillis - startTimeMs);

	// in this case, we have already finished fading, and can stop here.
	if (timeFadingMs >= runtimeMs) {
		elapsedPercentage = 1;
		// when it's done, we still have to set it to done for the last frame.
		// so opacity is set to 255.
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.opacity[i] = UINT8_MAX;
		}
		this->state = LedPipelineRunningState::DONE;
		return;
	} else {
		this->state = LedPipelineRunningState::RUNNING;
		elapsedPercentage = (float)timeFadingMs / (float)runtimeMs;
	}

	float opacityMultiplier = smoothingFunction(timeFadingMs, 0, runtimeMs, 0, UINT8_MAX);

	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		tempData.opacity[i] = opacityMultiplier * 255;
	}
}

void FadeIn::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
}


RandomFadeIn::RandomFadeIn(const unsigned long minRuntimeMs, const unsigned long maxRuntimeMs,
						   SamplingFunction samplingFunction, SmoothingFunction smoothingFunction,
						   BlendingMode blendingMode) :
	LedPipelineStage(blendingMode), RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction),
	smoothingFunction(smoothingFunction) {}

void RandomFadeIn::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->sampleRuntime();
		LPLogger::log(String("running random fade in effect for ") + this->runtimeMs + " ms");
		this->state = LedPipelineRunningState::RUNNING;
	}

	unsigned long currentTimeMillis = millis();

	unsigned long timeFadingMs = (currentTimeMillis - startTimeMs);

	// in this case, we have already finished fading, and can stop here.
	if (timeFadingMs >= runtimeMs) {
		LPLogger::log("done running random fade in effect.");
		elapsedPercentage = 1;
		// when it's done, we still have to set it to done for the last frame.
		// so opacity is set to 255.
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.opacity[i] = UINT8_MAX;
		}
		this->state = LedPipelineRunningState::DONE;
		return;
	} else {
		this->state = LedPipelineRunningState::RUNNING;
		elapsedPercentage = static_cast<float>(timeFadingMs) / static_cast<float>(runtimeMs);
	}

	const float currentOpacity = smoothingFunction(timeFadingMs, 0, runtimeMs, 0, UINT8_MAX);

	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		tempData.opacity[i] = currentOpacity;
	}
}

void RandomFadeIn::reset() {
	LedPipelineStage::reset();
	RandomTimedEffect::resetTimer();
}
