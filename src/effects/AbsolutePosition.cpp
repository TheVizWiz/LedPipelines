#include "effects/AbsolutePosition.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

AbsolutePosition::AbsolutePosition(LedPipelineStage* stage, Value<float> position)
	: WrapperEffect(stage), position(std::move(position)) {
	this->position.sample();
}

void AbsolutePosition::reset() {
	WrapperEffect::reset();
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
	result += jsonRawField("position", position.describeValue());
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledPosition", position.value));
	result += "}";
	return result;
}
#endif
