#include "effects/OpacityScale.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

OpacityScale::OpacityScale(LedPipelineStage* stage, Value<uint8_t> maxOpacity)
	: WrapperEffect(stage), maxOpacity(std::move(maxOpacity)) {
	this->maxOpacity.sample();
}

void OpacityScale::reset() {
	WrapperEffect::reset();
	this->maxOpacity.sample();
}

void OpacityScale::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	this->stage->calculate(startIndex, tempData);
	const uint8_t maxOpacity = this->maxOpacity.get();
	for (int i = 0; i < tempData.size; i++) {
		tempData.data[i].a = (tempData.data[i].a * maxOpacity) / 255;
	}

	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string OpacityScale::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("OpacityScale");
	result += jsonRawField("maxOpacity", maxOpacity.describeValue());
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledMaxOpacity", (int)maxOpacity.value));
	result += "}";
	return result;
}
#endif
