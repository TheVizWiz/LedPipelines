#include "effects/Solid.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Solid::Solid(Value<RGBA> color, BlendingMode blendingMode) : LedPipelineStage(blendingMode), color(std::move(color)) {
	// Sample the color once for this lifecycle, so this effect gets its own draw off the (possibly shared) recipe -
	// constant providers always yield the same value; random ones draw one. The result is cached in color.value.
	this->color.sample();
	this->state = LedPipelineRunningState::RUNNING;
}

void Solid::reset() {
	LedPipelineStage::reset();
	// Re-roll the color for the next run. For a constant this is a no-op; for a RandomColor/RandomColorSet it draws a
	// fresh color, which is the "sample once per lifecycle, resample on reset" contract.
	this->color.sample();
}

void Solid::calculate(const float startIndex, TemporaryLedData& tempData) {
	// A source has no lifecycle of its own - it just always draws - but it must still report RUNNING once it starts
	// rendering, not sit at NOT_STARTED. reset() drops every stage back to NOT_STARTED; without re-arming here, a Solid
	// would render correctly yet perpetually report NOT_STARTED. It never reaches DONE (a Solid never finishes on its
	// own), which is exactly why a Loop around a bare Solid loops forever - unchanged by this.
	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	const RGBA sampledColor = this->color.value;

	// A Solid fills the entire strip regardless of where it's positioned, so clamp the start to the strip bounds.
	// Without this, a negative startIndex (e.g. from a Moving/Shift wrapper) would start the loop at a negative index.
	int start = startIndex < 0 ? 0 : (int)startIndex;
	for (int i = start; i < TemporaryLedData::size; i++) {
		// The pixel's opacity is the color's own alpha (set() honors it when no explicit opacity is passed).
		tempData.set(i, sampledColor, sampledColor.a);
	}
}

SolidSegment::SolidSegment(Value<RGBA> color, Value<float> length, BlendingMode blendingMode)
	: Solid(std::move(color), blendingMode), length(std::move(length)) {
	// Sample the length once for this lifecycle alongside the color (Solid's ctor samples the color).
	this->length.sample();
}

void SolidSegment::reset() {
	Solid::reset();
	// Re-roll the length for the next run, mirroring the color re-roll in Solid::reset.
	this->length.sample();
}

void SolidSegment::calculate(float startIndex, TemporaryLedData& tempData) {
	// See Solid::calculate - a source re-arms to RUNNING once it starts rendering. SolidSegment overrides calculate()
	// rather than delegating, so it needs the same guard.
	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	const RGBA sampledColor = this->color.value;
	const float length = this->length.value;

	const float endIndex = startIndex + length;

	const int startIndexFloor = (int)startIndex;
	const int endIndexFloor = (int)endIndex;

	/**
	 * Two cases:
	 *
	 * if the start and end of the segment are in the same pixel, then we need to only light up that pixel, and only
	 * partially based on how large the pixel value is.
	 *
	 * if the start is on a different pixel than the end, we can light up the first pixel partially, then light up
	 * all pixels between start and end completely, and then light up the last pixel partially as well.
	 *
	 * Opacity is the color's own alpha (color.a); a partially-covered end pixel scales that alpha by its coverage.
	 */
	if (startIndexFloor == endIndexFloor) {
		// both are on the same pixel, we can light it up partially.
		const float amountToLightUp = length;
		tempData.set(startIndexFloor, sampledColor * amountToLightUp, sampledColor.a);
	} else {
		const float amountToLightUpFirstPixel = (1 - (startIndex - startIndexFloor));
		const float amountToLightUpLastPixel = (endIndex - endIndexFloor);

		if (amountToLightUpFirstPixel != 0) {
			tempData.set(startIndexFloor, sampledColor, sampledColor.a * amountToLightUpFirstPixel);
		}

		for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
			tempData.set(i, sampledColor, sampledColor.a);
		}

		if (amountToLightUpLastPixel != 0) {
			tempData.set(endIndexFloor, sampledColor, sampledColor.a * amountToLightUpLastPixel);
		}
	}
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Solid::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Solid");
	// Config emits the color RECIPE: a constant renders as its plain {r,g,b,a}, a random one as a {"provider":...} spec.
	// State emits the currently cached (sampled) color, so a random shows what it resolved to.
	result += jsonRawField("color", color.describeValue());
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("sampledColor", color.value));
	result += "}";
	return result;
}

std::string SolidSegment::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("SolidSegment");
	result += jsonRawField("color", color.describeValue());
	result += jsonRawField("length", length.describeValue());
	if (includeState) {
		result += jsonStateObject(
			jsonStateHeader() + jsonField("sampledColor", color.value) + jsonField("sampledLength", length.value));
	}
	result += "}";
	return result;
}
#endif
