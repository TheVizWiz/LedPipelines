#include "effects/Flip.h"


using namespace ledpipelines::effects;

Flip::Flip(LedPipelineStage* stage, long min, long max) :
	WrapperEffect(stage), min(std::min(min, max)), max(std::max(min, max)) {}


void Flip::calculate(float startIndex, ledpipelines::TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	stage->calculate(startIndex, tempData);

	// Mirror the region [min, max] about its true center. Pixel i maps to (min + max - i). Looping up to the midpoint
	// swaps each pair once; when (min + max) is even the exact center pixel maps to itself and is left untouched.
	for (int i = min; i < (min + max) / 2; i++) {
		int mirror = min + max - i;

		auto leftOpacity = tempData.getOpacity(i);
		auto leftRGB = tempData.get(i);

		tempData.set(i, tempData.get(mirror), tempData.getOpacity(mirror));
		tempData.set(mirror, leftRGB, leftOpacity);
	}
	this->state = this->stage->state;
}
