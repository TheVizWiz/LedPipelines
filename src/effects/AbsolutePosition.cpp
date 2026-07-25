#include "effects/AbsolutePosition.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

AbsolutePosition::AbsolutePosition(LedPipelineStage* stage, float position)
	: WrapperEffect(stage), position(position) {}

void AbsolutePosition::calculate(float startIndex, TemporaryLedData& tempData) {
	this->stage->calculate(position, tempData);
	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string AbsolutePosition::toJson(bool includeState) {

	std::string result = "{";

	result += jsonHeader("AbsolutePosition");
	result += jsonField("position", position);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	result += "}";
	return result;
}
#endif
