#include "effects/Solid.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Solid::Solid(Value<RGBA> color, BlendingMode blendingMode) : LedPipelineStage(blendingMode), color(std::move(color)) {
	this->color.sample();
	this->state = LedPipelineRunningState::RUNNING;
}

void Solid::reset() {
	LedPipelineStage::reset();
	this->color.sample();
}

void Solid::calculate(const float startIndex, TemporaryLedData& tempData) {
	// reset() drops every stage back to NOT_STARTED, so re-arm here. A Solid never reaches DONE, which is why a Loop
	// around a bare Solid loops forever.
	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	const RGBA sampledColor = this->color.get();

	// Clamp so a negative startIndex (e.g. from a Moving/Shift wrapper) doesn't start the loop at a negative index.
	int start = startIndex < 0 ? 0 : (int)startIndex;
	for (int i = start; i < TemporaryLedData::size; i++) {
		tempData.set(i, sampledColor, sampledColor.a);
	}
}

SolidSegment::SolidSegment(Value<RGBA> color, Value<float> length, BlendingMode blendingMode)
	: Solid(std::move(color), blendingMode), length(std::move(length)) {
	this->length.sample();
}

void SolidSegment::reset() {
	Solid::reset();
	this->length.sample();
}

void SolidSegment::calculate(float startIndex, TemporaryLedData& tempData) {
	// Overrides calculate() rather than delegating, so it needs the same re-arm as Solid::calculate.
	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	const RGBA sampledColor = this->color.get();
	const float length = this->length.get();

	const float endIndex = startIndex + length;

	const int startIndexFloor = (int)startIndex;
	const int endIndexFloor = (int)endIndex;

	if (startIndexFloor == endIndexFloor) {
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
