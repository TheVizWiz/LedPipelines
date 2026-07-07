#include "effects/FadeOut.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


FadeOut::FadeOut(unsigned long runtimeMs, SmoothingFunction smoothingFunction, BlendingMode blendingMode) :
	LedPipelineStage(blendingMode), TimedEffect(runtimeMs), smoothingFunction(smoothingFunction) {}

void FadeOut::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	unsigned long currentTimeMillis = millis();

	unsigned long timeFadingMs = (currentTimeMillis - startTimeMs);

	// in this case, we have already finished fading, and can stop here.
	if (timeFadingMs >= runtimeMs) {
		// when it's done, we still have to set it to done for the last frame.
		// so opacity is set to 0.
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.opacity[i] = 0;
		}
		elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}
	elapsedPercentage = (float)timeFadingMs / (float)runtimeMs;

	float opacityMultiplier = smoothingFunction(timeFadingMs, 0, runtimeMs, UINT8_MAX, 0);

	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		tempData.opacity[i] = (tempData.opacity[i] * opacityMultiplier) / 255;
	}
}

void FadeOut::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
}

RandomFadeOut::RandomFadeOut(unsigned long minRuntimeMs, unsigned long maxRuntimeMs, SamplingFunction samplingFunction,
							 SmoothingFunction smoothingFunction, BlendingMode blendingMode) :
	LedPipelineStage(blendingMode), RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction),
	smoothingFunction(smoothingFunction) {}


void RandomFadeOut::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->sampleRuntime();
		LPLogger::log(String("running random fade out effect for ") + this->runtimeMs + " ms");
		this->state = LedPipelineRunningState::RUNNING;
	}

	unsigned long currentTimeMillis = millis();

	float timeFadingMs = (currentTimeMillis - startTimeMs);

	// in this case, we have already finished fading, and can stop here.
	if (timeFadingMs >= runtimeMs) {
		LPLogger::log("done running random fade out effect.");

		// when it's done, we still have to set it to done for the last frame.
		// so opacity is set to 0.
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.opacity[i] = 0;
		}
		elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	elapsedPercentage = (float)timeFadingMs / (float)runtimeMs;

	float currentOpacity = smoothingFunction(timeFadingMs, 0, runtimeMs, UINT8_MAX, 0);

	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		tempData.opacity[i] = currentOpacity;
	}
}

void RandomFadeOut::reset() {
	LedPipelineStage::reset();
	RandomTimedEffect::resetTimer();
}
