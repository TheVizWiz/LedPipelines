#include <utility>

#include "effects/Moving.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Moving::Moving(
	LedPipelineStage *stage,
	const unsigned long runtimeMs,
	const float startPosition,
	const float endPosition,
	SmoothingFunction smoothingFunction)
	: WrapperEffect(stage),
	  TimedEffect(runtimeMs),
	  currentPosition(startPosition),
	  startPosition(startPosition),
	  endPosition(endPosition),
	  smoothingFunction(smoothingFunction) {}


void Moving::calculate(float startIndex, TemporaryLedData &tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->elapsedPercentage = (float) (millis() - this->startTimeMs) / (float) runtimeMs;

	this->currentPosition = smoothingFunction(elapsedPercentage, startPosition, endPosition);

	this->stage->calculate(startIndex + currentPosition, tempData);

	if (this->stage->state == LedPipelineRunningState::DONE || this->elapsedPercentage >= 1) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void Moving::reset() {
	WrapperEffect::reset();
	this->currentPosition = this->startPosition;
}
