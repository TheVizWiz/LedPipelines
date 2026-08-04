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
	// Sample the fade curve once for this lifecycle (a constant yields the same preset; a random one draws one).
	this->smoothingFunction.sample();
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

	if (timeFadingMs >= runtimeMs.value) {
		// Ramp complete: full opacity, so leave the inner effect's opacity untouched (multiplier == 1). By default we do
		// NOT go DONE here - FadeIn is a pass-through once faded in, and terminating would hide the wrapped content.
		elapsedPercentage = 1;

		// terminateOnComplete opts out of that pass-through: finish the instant the ramp completes rather than waiting
		// for the inner. This is what lets a Series advance to the next stage (e.g. a FadeOut) the moment the fade-in
		// ends. The inner already rendered above at full opacity, so this frame still shows fully faded-in content.
		if (this->terminateOnComplete) {
			this->state = LedPipelineRunningState::DONE;
			return;
		}
	} else {
		elapsedPercentage = (float)timeFadingMs / (float)runtimeMs.value;
		float opacityMultiplier = smoothingFunction.value(timeFadingMs, 0, runtimeMs.value, 0, UINT8_MAX);
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.data[i].a = (tempData.data[i].a * opacityMultiplier) / 255;
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
	// Re-roll the fade curve for the next run (no-op for a constant; fresh draw for a random one).
	smoothingFunction.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string FadeIn::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("FadeIn");
	// Config emits each input's RECIPE (a constant, or a random spec); sampled values are state (jsonTimedEffect).
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
