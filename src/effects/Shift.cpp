#include "effects/Shift.h"

#include <cmath>  // std::round

using namespace ledpipelines;

using namespace ledpipelines::effects;

Shift::Shift(LedPipelineStage* stage, Value<float> offset, bool useWholePixels)
	: WrapperEffect(stage), offset(std::move(offset)), useWholePixels(useWholePixels) {
	// Sample the offset once for this lifecycle, so this effect gets its own draw off the (possibly shared) recipe -
	// a constant always yields the same value; a RandomRange draws one. The result is cached in offset.value.
	this->offset.sample();
}

float Shift::effectiveOffset() const {
	// Snap to an integer pixel so the shifted copy lands exactly on LEDs (no fractional-coverage feathering across two
	// pixels) when useWholePixels is set. Applied on read rather than baked into offset.value, so the raw sampled value
	// is preserved for serialization.
	return useWholePixels ? std::round(offset.value) : offset.value;
}

void Shift::calculate(const float startIndex, TemporaryLedData& tempData) {
	this->stage->calculate(startIndex + effectiveOffset(), tempData);

	// Pass through the inner's lifecycle: Shift adds no timing of its own, so it is DONE exactly when its inner is.
	// Without this a Shift-wrapped effect never reports DONE, which (for example) stops a Spawner from ever reaping it.
	this->state = this->stage->state;
}

void Shift::reset() {
	WrapperEffect::reset();
	// Re-roll the offset for the next run. For a constant this is a no-op; for a RandomRange it draws a fresh offset -
	// the "sample once per lifecycle, resample on reset" contract (mirrors the old RandomShift re-sampling).
	this->offset.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Shift::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Shift");
	// Config emits the offset RECIPE (a constant, or a RandomRange spec); the sampled offset is state.
	result += jsonRawField("offset", offset.describeValue());
	result += jsonField("useWholePixels", useWholePixels);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledOffset", effectiveOffset()));
	result += "}";
	return result;
}
#endif
