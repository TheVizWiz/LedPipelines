#include "effects/Shift.h"

#include <cmath>

using namespace ledpipelines;

using namespace ledpipelines::effects;

Shift::Shift(LedPipelineStage* stage, Value<float> offset, bool useWholePixels)
	: WrapperEffect(stage), offset(std::move(offset)), useWholePixels(useWholePixels) {
	this->offset.sample();
}

float Shift::effectiveOffset() const {
	// Applied on read rather than baked into offset.value, so the raw sampled value is preserved for serialization.
	return useWholePixels ? std::round(offset.value) : offset.value;
}

void Shift::calculate(const float startIndex, TemporaryLedData& tempData) {
	this->stage->calculate(startIndex + effectiveOffset(), tempData);

	// Without this a Shift-wrapped effect never reports DONE, which (for example) stops a Spawner from ever reaping it.
	this->state = this->stage->state;
}

void Shift::reset() {
	WrapperEffect::reset();
	this->offset.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Shift::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Shift");
	result += jsonRawField("offset", offset.describeValue());
	result += jsonField("useWholePixels", useWholePixels);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledOffset", effectiveOffset()));
	result += "}";
	return result;
}
#endif
