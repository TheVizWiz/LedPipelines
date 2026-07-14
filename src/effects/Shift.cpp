#include "effects/Shift.h"

#include <cmath>  // std::round

using namespace ledpipelines;

using namespace ledpipelines::effects;

void Shift::calculate(const float startIndex, TemporaryLedData& tempData) {
	this->stage->calculate(startIndex + offset, tempData);

	// Pass through the inner's lifecycle: Shift adds no timing of its own, so it is DONE exactly when its inner is.
	// Without this a Shift-wrapped effect never reports DONE, which (for example) stops a Spawner from ever reaping it.
	this->state = this->stage->state;
}

Shift::Shift(LedPipelineStage* stage, const float offset) : WrapperEffect(stage), offset(offset) {}


RandomShift::RandomShift(
	LedPipelineStage* stage,
	const float minOffset,
	const float maxOffset,
	SamplingFunction samplingFunction,
	bool useWholePixels
)
	: WrapperEffect(stage), minOffset(minOffset), maxOffset(maxOffset), samplingFunction(samplingFunction),
	  useWholePixels(useWholePixels), offset(0) {}

void RandomShift::calculate(const float startIndex, TemporaryLedData& tempData) {
	// Sample the offset once, on the first run, then keep it stable for the life of this run. reset() (below) drops us
	// back to NOT_STARTED so a subsequent run re-samples a fresh offset.
	if (!this->sampled) {
		this->offset = this->samplingFunction(this->minOffset, this->maxOffset);
		// Snap to an integer pixel so the shifted copy lands exactly on LEDs (no fractional-coverage feathering across
		// two pixels). Off by default; a caller sets it when it wants crisp per-pixel placement.
		if (this->useWholePixels) this->offset = std::round(this->offset);
		this->sampled = true;
	}

	this->stage->calculate(startIndex + this->offset, tempData);

	// Pass through the inner's lifecycle so RandomShift reports DONE when its inner finishes (see Shift::calculate).
	// This is what lets a Spawner reap a Shift-wrapped, terminating child instead of piling children up to maxChildren
	// and then never spawning again.
	this->state = this->stage->state;
}

void RandomShift::reset() {
	WrapperEffect::reset();
	this->sampled = false;
}
