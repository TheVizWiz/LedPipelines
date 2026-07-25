#include "effects/Solid.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Solid::Solid(const RGBA color, BlendingMode blendingMode) : LedPipelineStage(blendingMode), color(color) {
	this->state = LedPipelineRunningState::RUNNING;
}

void Solid::calculate(const float startIndex, TemporaryLedData& tempData) {
	// A Solid fills the entire strip regardless of where it's positioned, so clamp the start to the strip bounds.
	// Without this, a negative startIndex (e.g. from a Moving/Shift wrapper) would start the loop at a negative index.
	int start = startIndex < 0 ? 0 : (int)startIndex;
	for (int i = start; i < TemporaryLedData::size; i++) {
		// The pixel's opacity is the color's own alpha (set() honors it when no explicit opacity is passed).
		tempData.set(i, color, color.a);
	}
}

SolidSegment::SolidSegment(const RGBA color, float length, BlendingMode blendingMode)
	: Solid(color, blendingMode), length(length) {}

void SolidSegment::calculate(float startIndex, TemporaryLedData& tempData) {
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
		tempData.set(startIndexFloor, color * amountToLightUp, color.a);
	} else {
		const float amountToLightUpFirstPixel = (1 - (startIndex - startIndexFloor));
		const float amountToLightUpLastPixel = (endIndex - endIndexFloor);

		if (amountToLightUpFirstPixel != 0) {
			tempData.set(startIndexFloor, color, color.a * amountToLightUpFirstPixel);
		}

		for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
			tempData.set(i, color, color.a);
		}

		if (amountToLightUpLastPixel != 0) {
			tempData.set(endIndexFloor, color, color.a * amountToLightUpLastPixel);
		}
	}
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Solid::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Solid");
	result += jsonField("color", color);
	result += "}";
	return result;
}

std::string SolidSegment::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("SolidSegment");
	result += jsonField("color", color);
	result += jsonField("length", length);
	result += "}";
	return result;
}
#endif
