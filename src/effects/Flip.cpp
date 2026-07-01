#include "effects/Flip.h"


using namespace ledpipelines::effects;

Flip::Flip(LedPipelineStage* stage, long minIndex, long maxIndex) :
	WrapperEffect(stage), minIndex(std::min(minIndex, maxIndex)), maxIndex(std::max(minIndex, maxIndex)) {}


void Flip::calculate(float startIndex, ledpipelines::TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	stage->calculate(startIndex, tempData);

	// Mirror the region [minIndex, maxIndex] about its true center. Pixel i maps to (minIndex + maxIndex - i). Looping
	// up to the midpoint swaps each pair once; when (minIndex + maxIndex) is even the exact center pixel maps to itself
	// and is left untouched.
	for (int i = minIndex; i < (minIndex + maxIndex) / 2; i++) {
		int mirror = minIndex + maxIndex - i;

		auto leftOpacity = tempData.getOpacity(i);
		auto leftRGB = tempData.get(i);

		tempData.set(i, tempData.get(mirror), tempData.getOpacity(mirror));
		tempData.set(mirror, leftRGB, leftOpacity);
	}
	this->state = this->stage->state;
}
