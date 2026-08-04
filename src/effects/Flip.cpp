#include "effects/Flip.h"

#include <algorithm>  // std::min / std::max


using namespace ledpipelines::effects;

Flip::Flip(LedPipelineStage* stage, Value<long> minIndex, Value<long> maxIndex)
	: WrapperEffect(stage), minIndex(std::move(minIndex)), maxIndex(std::move(maxIndex)) {
	// Sample the bounds once for this lifecycle (constants yield the same value; randoms draw one).
	this->minIndex.sample();
	this->maxIndex.sample();
}


void Flip::reset() {
	WrapperEffect::reset();
	// Re-roll the bounds for the next run (no-ops for constants; fresh draws for randoms).
	this->minIndex.sample();
	this->maxIndex.sample();
}


void Flip::calculate(float startIndex, ledpipelines::TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	stage->calculate(startIndex, tempData);

	// Normalize the sampled bounds so lo <= hi regardless of the order given (or drawn). This was previously done in the
	// constructor; with sampled Values it happens here on the current draw.
	const long lo = std::min(minIndex.value, maxIndex.value);
	const long hi = std::max(minIndex.value, maxIndex.value);

	// Mirror the region [lo, hi] about its true center. Pixel i maps to (lo + hi - i). Looping up to the midpoint swaps
	// each pair once; when (lo + hi) is even the exact center pixel maps to itself and is left untouched.
	for (int i = lo; i < (lo + hi) / 2; i++) {
		int mirror = lo + hi - i;

		auto leftOpacity = tempData.getOpacity(i);
		auto leftRGB = tempData.get(i);

		tempData.set(i, tempData.get(mirror), tempData.getOpacity(mirror));
		tempData.set(mirror, leftRGB, leftOpacity);
	}
	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Flip::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Flip");
	// Config emits each input's RECIPE; sampled values are state.
	result += jsonRawField("minIndex", minIndex.describeValue());
	result += jsonRawField("maxIndex", maxIndex.describeValue());
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) {
		result += jsonStateObject(
			jsonStateHeader() + jsonField("sampledMinIndex", (int)minIndex.value) +
			jsonField("sampledMaxIndex", (int)maxIndex.value));
	}
	result += "}";
	return result;
}
#endif
