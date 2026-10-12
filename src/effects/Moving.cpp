#include <utility>

#include "effects/Moving.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Moving::Moving(
	LedPipelineStage* stage,
	Value<unsigned long> runtimeMs,
	Value<float> startPosition,
	Value<float> endPosition,
	Value<SmoothingFunction> smoothingFunction
)
	: WrapperEffect(stage),
	  TimedEffect(std::move(runtimeMs)),
	  startPosition(std::move(startPosition)),
	  endPosition(std::move(endPosition)),
	  smoothingFunction(std::move(smoothingFunction)) {
	this->startPosition.sample();
	this->endPosition.sample();
	this->smoothingFunction.sample();
	this->currentPosition = this->startPosition.get();
}


void Moving::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->elapsedPercentage = (float)elapsedMs() / (float)runtimeMs.get();

	// smoothingFunction clamps the percentage to [0, 1], so once the move completes the inner stays pinned at
	// endPosition; by default only the inner reaching DONE ends this effect.
	this->currentPosition = smoothingFunction.get()(elapsedPercentage, startPosition.get(), endPosition.get());

	this->stage->calculate(startIndex + currentPosition, tempData);

	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}

	if (this->terminateOnComplete && this->elapsedPercentage >= 1) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void Moving::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
	this->startPosition.sample();
	this->endPosition.sample();
	this->smoothingFunction.sample();
	this->currentPosition = this->startPosition.get();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Moving::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Moving");
	result += jsonRawField("startPosition", startPosition.describeValue());
	result += jsonRawField("endPosition", endPosition.describeValue());
	result += jsonRawField("smoothingFunction", smoothingFunction.describeValue());
	result += jsonRawField("runtimeMs", runtimeMs.describeValue());
	result += jsonRawField("delayMs", delayMs.describeValue());
	result += jsonField("terminateOnComplete", terminateOnComplete);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonTimedEffect() + jsonField("currentPosition", currentPosition));
	result += "}";
	return result;
}
#endif
