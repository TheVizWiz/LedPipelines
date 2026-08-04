#include "effects/AbsolutePosition.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

AbsolutePosition::AbsolutePosition(LedPipelineStage* stage, Value<float> position)
	: WrapperEffect(stage), position(std::move(position)) {
	// Sample the position once for this lifecycle (a constant yields the same value; a random one draws one).
	this->position.sample();
}

void AbsolutePosition::reset() {
	WrapperEffect::reset();
	// Re-roll the position for the next run (no-op for a constant; fresh draw for a random one).
	this->position.sample();
}

void AbsolutePosition::calculate(float startIndex, TemporaryLedData& tempData) {
	this->stage->calculate(position.value, tempData);
	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string AbsolutePosition::toJson(bool includeState) {

	std::string result = "{";

	result += jsonHeader("AbsolutePosition");
	// Config emits the position RECIPE; the sampled position is state.
	result += jsonRawField("position", position.describeValue());
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledPosition", position.value));
	result += "}";
	return result;
}
#endif
