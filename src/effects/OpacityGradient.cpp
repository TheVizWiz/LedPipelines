#include "effects/OpacityGradient.h"

#include <cmath>

using namespace ledpipelines;
using namespace ledpipelines::effects;


OpacityGradient::OpacityGradient(
	LedPipelineStage* stage,
	Value<float> startIndex,
	Value<float> endIndex,
	SmoothingFunction smoothingFunction
)
	: WrapperEffect(stage),
	  startIndex(std::move(startIndex)),
	  endIndex(std::move(endIndex)),
	  smoothingFunction(smoothingFunction) {
	this->startIndex.sample();
	this->endIndex.sample();
}


void OpacityGradient::reset() {
	WrapperEffect::reset();
	this->startIndex.sample();
	this->endIndex.sample();
}


void OpacityGradient::calculateForwardGradient(float startPos, float endIndex, TemporaryLedData& tempData) const {
	const float startIndex = startPos;
	int startIndexFloor = (int)startIndex;
	int endIndexFloor = (int)endIndex;

	/**
	 * Each pixel's opacity is multiplied by the average of the gradient value at its left and right bounds. If
	 * start and end share a pixel, that average is 0.5.
	 */
	if (startIndexFloor == endIndexFloor) {
		float amountToFadePixel = 0.5;

		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * amountToFadePixel
		);
	} else {
		float firstPixelLeftBound = 0;
		float firstPixelRightBound = smoothingFunction(floor(startIndex + 1), startIndex, endIndex, 0, 1);
		float firstPixelFadeAmount = (firstPixelLeftBound + firstPixelRightBound) / 2;

		float lastPixelLeftBound = smoothingFunction(floor(endIndex), startIndex, endIndex, 0, 1);
		float lastPixelRightBound = 1;
		float lastPixelFadeAmount = (lastPixelLeftBound + lastPixelRightBound) / 2;

		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * firstPixelFadeAmount
		);

		// Evaluating at the pixel center gives the average of its left and right bounds in one call.
		for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
			float pixelAverageFadeAmount = smoothingFunction(i + 0.5, startIndex, endIndex, 0, 1);

			tempData.set(i, tempData.get(i), tempData.getOpacity(i) * pixelAverageFadeAmount);
		}

		tempData
			.set(endIndexFloor, tempData.get(endIndexFloor), tempData.getOpacity(endIndexFloor) * lastPixelFadeAmount);
	}
}


void OpacityGradient::calculateBackwardGradient(float startPos, float endIndex, TemporaryLedData& tempData) {
	// Backwards: same as forward with the start and end roles flipped.
	const float startIndex = startPos;
	int startIndexFloor = (int)startIndex;
	int endIndexFloor = (int)endIndex;

	if (startIndexFloor == endIndexFloor) {
		float amountToFadePixel = 0.5;

		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * amountToFadePixel
		);
	} else {
		float firstPixelLeftBound = 1;
		float firstPixelRightBound = smoothingFunction(floor(startIndex + 1), endIndex, startIndex, 1, 0);
		float firstPixelFadeAmount = (firstPixelLeftBound + firstPixelRightBound) / 2;

		float lastPixelLeftBound = smoothingFunction(floor(endIndex), endIndex, startIndex, 1, 0);
		float lastPixelRightBound = 0;
		float lastPixelFadeAmount = (lastPixelLeftBound + lastPixelRightBound) / 2;

		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * firstPixelFadeAmount
		);

		for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
			float pixelAverageFadeAmount = smoothingFunction(i + 0.5, endIndex, startIndex, 1, 0);

			tempData.set(i, tempData.get(i), tempData.getOpacity(i) * pixelAverageFadeAmount);
		}

		tempData
			.set(endIndexFloor, tempData.get(endIndexFloor), tempData.getOpacity(endIndexFloor) * lastPixelFadeAmount);
	}
}

void OpacityGradient::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->stage->calculate(startIndex, tempData);

	// Only the start is offset by this stage's pipeline position; the end is passed un-offset.
	if (this->startIndex.get() > this->endIndex.get())
		calculateBackwardGradient(startIndex + this->startIndex.get(), this->endIndex.get(), tempData);
	else
		calculateForwardGradient(startIndex + this->startIndex.get(), this->endIndex.get(), tempData);

	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string OpacityGradient::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("OpacityGradient");
	result += jsonRawField("startIndex", startIndex.describeValue());
	result += jsonRawField("endIndex", endIndex.describeValue());
	result += jsonField("smoothingFunction", smoothingFunction);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) {
		result += jsonStateObject(
			jsonStateHeader() + jsonField("sampledStartIndex", startIndex.value) +
			jsonField("sampledEndIndex", endIndex.value));
	}
	result += "}";
	return result;
}
#endif
