#include <utility>

#include "effects/Moving.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Moving::Moving(
	LedPipelineStage* stage,
	const unsigned long runtimeMs,
	const float startPosition,
	const float endPosition,
	SmoothingFunction smoothingFunction
)
	: WrapperEffect(stage),
	  TimedEffect(runtimeMs),
	  currentPosition(startPosition),
	  startPosition(startPosition),
	  endPosition(endPosition),
	  smoothingFunction(smoothingFunction) {}


void Moving::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	// elapsedMs() holds at 0 during the lead-in delay, so a delayed Moving sits at its startPosition until the delay
	// elapses, then begins moving.
	this->elapsedPercentage = (float)elapsedMs() / (float)runtimeMs;

	// smoothingFunction clamps the percentage to [0, 1], so once the movement timer completes the inner is pinned
	// exactly at endPosition. We keep rendering it there until the inner itself finishes - the movement running out
	// does NOT end this effect; only the inner reaching DONE does. This lets the inner keep animating at its final
	// destination after the move completes.
	this->currentPosition = smoothingFunction(elapsedPercentage, startPosition, endPosition);

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
	this->currentPosition = this->startPosition;
}
