#include "effects/Flip.h"

#include <algorithm>


using namespace ledpipelines::effects;

Flip::Flip(LedPipelineStage* stage, Value<long> minIndex, Value<long> maxIndex)
	: WrapperEffect(stage), minIndex(std::move(minIndex)), maxIndex(std::move(maxIndex)) {
	this->minIndex.sample();
	this->maxIndex.sample();
}


void Flip::reset() {
	WrapperEffect::reset();
	this->minIndex.sample();
	this->maxIndex.sample();
}


void Flip::calculate(float startIndex, ledpipelines::TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	stage->calculate(startIndex, tempData);

	const long lo = std::min(minIndex.get(), maxIndex.get());
	const long hi = std::max(minIndex.get(), maxIndex.get());

	for (long i = lo, mirror = hi; i < mirror; i++, mirror--) {
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
