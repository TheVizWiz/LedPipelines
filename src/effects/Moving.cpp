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
	// Sample the position/smoothing inputs once for this lifecycle (constants yield the same value; randoms draw one).
	this->startPosition.sample();
	this->endPosition.sample();
	this->smoothingFunction.sample();
	// currentPosition starts at the sampled start position.
	this->currentPosition = this->startPosition.value;
}


void Moving::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	// elapsedMs() holds at 0 during the lead-in delay, so a delayed Moving sits at its startPosition until the delay
	// elapses, then begins moving.
	this->elapsedPercentage = (float)elapsedMs() / (float)runtimeMs.value;

	// smoothingFunction clamps the percentage to [0, 1], so once the movement timer completes the inner is pinned
	// exactly at endPosition. We keep rendering it there until the inner itself finishes - the movement running out
	// does NOT end this effect; only the inner reaching DONE does. This lets the inner keep animating at its final
	// destination after the move completes.
	this->currentPosition = smoothingFunction.value(elapsedPercentage, startPosition.value, endPosition.value);

	this->stage->calculate(startIndex + currentPosition, tempData);

	if (this->stage->state == LedPipelineRunningState::DONE) {
		this->state = LedPipelineRunningState::DONE;
	}

	// By default Moving holds at endPosition and defers termination to the inner (see the comment above). With
	// terminateOnComplete set, it instead finishes as soon as its own move completes - the inner just rendered at
	// endPosition on this frame, so a Series can advance to the next stage the moment the move lands.
	if (this->terminateOnComplete && this->elapsedPercentage >= 1) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void Moving::reset() {
	WrapperEffect::reset();
	TimedEffect::resetTimer();
	// Re-roll the position/smoothing inputs for the next run (no-ops for constants; fresh draws for randoms).
	this->startPosition.sample();
	this->endPosition.sample();
	this->smoothingFunction.sample();
	this->currentPosition = this->startPosition.value;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Moving::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Moving");
	// Config emits each input's RECIPE (a constant, or a random spec); sampled values are state (below / jsonTimedEffect).
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
