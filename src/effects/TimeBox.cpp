#include "effects/TimeBox.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

TimeBox::TimeBox(LedPipelineStage* stage, Value<unsigned long> runtimeMs)
	: WrapperEffect(stage), TimedEffect(std::move(runtimeMs)) {}


void TimeBox::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		startTimeMs = millis();
	}

	// elapsedMs() holds at 0 during the lead-in delay, so a delayed TimeBox shows its inner for the delay before its
	// own runtime clock starts counting.
	unsigned long elapsedTime = elapsedMs();


	this->elapsedPercentage = (float)elapsedTime / (float)runtimeMs.value;
	this->stage->calculate(startIndex, tempData);
	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}

	if (elapsedTime >= runtimeMs.value) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}
}


void TimeBox::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string TimeBox::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("TimeBox");
	result += jsonRawField("runtimeMs", runtimeMs.describeValue());
	result += jsonRawField("delayMs", delayMs.describeValue());
	result += jsonField("terminateOnComplete", terminateOnComplete);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonTimedEffect());
	result += "}";
	return result;
}
#endif
