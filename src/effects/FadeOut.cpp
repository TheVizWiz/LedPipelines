#include "effects/FadeOut.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


FadeOut::FadeOut(LedPipelineStage* stage, unsigned long runtimeMs, SmoothingFunction smoothingFunction,
				 BlendingMode blendingMode) :
	WrapperEffect(stage), TimedEffect(runtimeMs), smoothingFunction(smoothingFunction) {
	this->blendingMode = blendingMode;
}

void FadeOut::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	// Render the wrapped effect first, then scale its opacity down by the fade ramp.
	this->stage->calculate(startIndex, tempData);

	// elapsedMs() is the time on our own timeline: 0 during the lead-in delay (so the content shows at full opacity),
	// then counting up once the delay elapses. This is what makes `content.wrap(FadeOut(1000).delayMs(5000))` hold the
	// content for 5s before ramping out.
	unsigned long timeFadingMs = elapsedMs();

	if (timeFadingMs >= runtimeMs) {
		// Ramp complete: fully faded out. Zero the buffer and finish - fading content away to nothing is the point of
		// FadeOut, so it terminates on its own timer rather than deferring to the inner effect.
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

	// Also finish if the wrapped effect finished before the fade ran out.
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void FadeOut::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
}

RandomFadeOut::RandomFadeOut(LedPipelineStage* stage, unsigned long minRuntimeMs, unsigned long maxRuntimeMs,
							 SamplingFunction samplingFunction, SmoothingFunction smoothingFunction,
							 BlendingMode blendingMode) :
	WrapperEffect(stage), RandomTimedEffect(minRuntimeMs, maxRuntimeMs, samplingFunction),
	smoothingFunction(smoothingFunction) {
	this->blendingMode = blendingMode;
}


void RandomFadeOut::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->sampleRuntime();
		LPLogger::log(String("running random fade out effect for ") + this->runtimeMs + " ms");
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->stage->calculate(startIndex, tempData);

	float timeFadingMs = elapsedMs();

	if (timeFadingMs >= runtimeMs) {
		LPLogger::log("done running random fade out effect.");
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

	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void RandomFadeOut::reset() {
	WrapperEffect::reset();
	RandomTimedEffect::resetTimer();
}
