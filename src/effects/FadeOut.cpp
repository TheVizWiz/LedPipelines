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
	this->smoothingFunction.sample();
	this->blendingMode = blendingMode;
}

void FadeOut::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->stage->calculate(startIndex, tempData);

	unsigned long timeFadingMs = elapsedMs();

	if (timeFadingMs >= runtimeMs.value) {
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

	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void FadeOut::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
	smoothingFunction.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string FadeOut::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("FadeOut");
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
