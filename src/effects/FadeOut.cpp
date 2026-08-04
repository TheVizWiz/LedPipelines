#include "effects/FadeOut.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


FadeOut::FadeOut(
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

	if (timeFadingMs >= runtimeMs.value) {
		// Ramp complete: fully faded out. Zero the buffer and finish - fading content away to nothing is the point of
		// FadeOut, so it terminates on its own timer rather than deferring to the inner effect.
		for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
			tempData.data[i].a = 0;
		}
		elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	elapsedPercentage = (float)timeFadingMs / (float)runtimeMs.value;
	float opacityMultiplier = smoothingFunction.value(timeFadingMs, 0, runtimeMs.value, UINT8_MAX, 0);
	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		tempData.data[i].a = (tempData.data[i].a * opacityMultiplier) / 255;
	}

	// Also finish if the wrapped effect finished before the fade ran out.
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void FadeOut::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
	// Re-roll the fade curve for the next run (no-op for a constant; fresh draw for a random one).
	smoothingFunction.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string FadeOut::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("FadeOut");
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
