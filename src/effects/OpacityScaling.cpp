#include "effects/OpacityScale.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

OpacityScale::OpacityScale(LedPipelineStage* stage, Value<uint8_t> maxOpacity)
	: WrapperEffect(stage), maxOpacity(std::move(maxOpacity)) {
	// Sample the ceiling once for this lifecycle (a constant yields the same value; a random one draws one).
	this->maxOpacity.sample();
}

void OpacityScale::reset() {
	WrapperEffect::reset();
	// Re-roll the ceiling for the next run (no-op for a constant; fresh draw for a random one).
	this->maxOpacity.sample();
}

void OpacityScale::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	this->stage->calculate(startIndex, tempData);
	const uint8_t maxOpacity = this->maxOpacity.value;
	for (int i = 0; i < tempData.size; i++) {
		tempData.data[i].a = (tempData.data[i].a * maxOpacity) / 255;
	}

	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string OpacityScale::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("OpacityScale");
	// Config emits the ceiling RECIPE; the sampled ceiling is state.
	result += jsonRawField("maxOpacity", maxOpacity.describeValue());
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledMaxOpacity", (int)maxOpacity.value));
	result += "}";
	return result;
}
#endif
