#include "effects/FadeIn.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


FadeIn::FadeIn(
	LedPipelineStage* stage,
	Value<unsigned long> runtimeMs,
	Value<SmoothingFunction> smoothingFunction,
	BlendingMode blendingMode
)
	: WrapperEffect(stage), TimedEffect(std::move(runtimeMs)), smoothingFunction(std::move(smoothingFunction)) {
	this->smoothingFunction.sample();
	this->blendingMode = blendingMode;
}

void FadeIn::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->stage->calculate(startIndex, tempData);

	unsigned long timeFadingMs = elapsedMs();

	if (timeFadingMs >= runtimeMs.get()) {
		elapsedPercentage = 1;

		// The inner already rendered above at full opacity, so this frame still shows fully faded-in content.
		if (this->terminateOnComplete) {
			this->state = LedPipelineRunningState::DONE;
			return;
		}
	} else {
		elapsedPercentage = (float)timeFadingMs / (float)runtimeMs.get();
		float opacityMultiplier = smoothingFunction.get()(timeFadingMs, 0, runtimeMs.get(), 0, UINT8_MAX);
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.data[i].a = (tempData.data[i].a * opacityMultiplier) / 255;
		}
	}

	// Don't copy the inner's state wholesale: an inner that reports NOT_STARTED every frame (e.g. Solid) would keep
	// re-initializing the fade timer, freezing the ramp.
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void FadeIn::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
	smoothingFunction.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string FadeIn::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("FadeIn");
	result += jsonRawField("smoothingFunction", smoothingFunction.describeValue());
	result += jsonRawField("runtimeMs", runtimeMs.describeValue());
	result += jsonRawField("delayMs", delayMs.describeValue());
	result += jsonField("terminateOnComplete", terminateOnComplete);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonTimedEffect());
	result += "}";
	return result;
}
#endif
