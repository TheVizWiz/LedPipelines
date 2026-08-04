#include "effects/OpacityGradient.h"

#include <cmath>  // floor

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
	// Sample the endpoints once for this lifecycle (constants yield the same value; randoms draw one).
	this->startIndex.sample();
	this->endIndex.sample();
}


void OpacityGradient::reset() {
	WrapperEffect::reset();
	// Re-roll the endpoints for the next run (no-ops for constants; fresh draws for randoms).
	this->startIndex.sample();
	this->endIndex.sample();
}


void OpacityGradient::calculateForwardGradient(float startPos, float endIndex, TemporaryLedData& tempData) const {
	// startPos is the offset-applied start position; endIndex is the sampled end value (passed in, matching the pre-Value
	// behavior where the helper read the raw endIndex member).
	const float startIndex = startPos;
	int startIndexFloor = (int)startIndex;
	int endIndexFloor = (int)endIndex;

	/**
	 *
	 * How we light up a pixel:
	 *
	 * We calculate the opacity at the left bound of the pixel, and the opacity at the right bound of the pixel.
	 * Then, the opacity of the pixel is multiplied by the average of the left and right bounds of the pixel.
	 *
	 * If the start and end are the same pixel, then set the left bound as the start and the right bound as the end,
	 * and the pixel will be lit up by that range. Since we always light up from 0 to 100%, if the left and right
	 * are on the same pixel, then we multiply the opacity by 50% (the average of left and right bounds).
	 *
	 * If the start and are not on the same pixel, we set:
	 * - left bound of the first pixel as start (0%), and right bound as the right bound.
	 * - left bound and right bound of every pixel between start and end as left and right bound.
	 * - left bound of the last pixel as left bound, and right bound as the end value (100%).
	 */
	if (startIndexFloor == endIndexFloor) {
		// both are on the same pixel, we can fade it partially. We fade it by 50% since the left and right bounds
		// are 0 and 1.
		float amountToFadePixel = 0.5;

		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * amountToFadePixel
		);
	} else {
		// first pixel calculations
		float firstPixelLeftBound = 0;
		float firstPixelRightBound = smoothingFunction(floor(startIndex + 1), startIndex, endIndex, 0, 1);
		float firstPixelFadeAmount = (firstPixelLeftBound + firstPixelRightBound) / 2;

		// last pixel calculations
		float lastPixelLeftBound = smoothingFunction(floor(endIndex), startIndex, endIndex, 0, 1);
		float lastPixelRightBound = 1;
		float lastPixelFadeAmount = (lastPixelLeftBound + lastPixelRightBound) / 2;


		// set first pixel
		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * firstPixelFadeAmount
		);


		// for every pixel in between, we calculate what the left and right bounds are, and average them.
		// to this, we use a clever trick: we know the left and right pixel values, and the average will be exactly
		// halfway, so we can just do left + 0.5. Then, the calculation will be much faster.
		for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
			float pixelAverageFadeAmount = smoothingFunction(i + 0.5, startIndex, endIndex, 0, 1);

			tempData.set(i, tempData.get(i), tempData.getOpacity(i) * pixelAverageFadeAmount);
		}

		// set last pixel
		tempData
			.set(endIndexFloor, tempData.get(endIndexFloor), tempData.getOpacity(endIndexFloor) * lastPixelFadeAmount);
	}
}


void OpacityGradient::calculateBackwardGradient(float startPos, float endIndex, TemporaryLedData& tempData) {
	// if the gradient is backwards, the left value is 1 and the right value is 0.
	// instead of doing complex math, we just flip how we are calculating start and end indices.
	// startPos is the offset-applied start position; endIndex is the sampled end value (see calculateForwardGradient).
	const float startIndex = startPos;
	int startIndexFloor = (int)startIndex;
	int endIndexFloor = (int)endIndex;

	/**
	 *
	 * How we light up a pixel:
	 *
	 * We calculate the opacity at the left bound of the pixel, and the opacity at the right bound of the pixel.
	 * Then, the opacity of the pixel is multiplied by the average of the left and right bounds of the pixel.
	 *
	 * If the start and end are the same pixel, then set the left bound as the start and the right bound as the end,
	 * and the pixel will be lit up by that range. Since we always light up from 0 to 100%, if the left and right
	 * are on the same pixel, then we multiply the opacity by 50% (the average of left and right bounds).
	 *
	 * If the start and are not on the same pixel, we set:
	 * - left bound of the first pixel as start (0%), and right bound as the right bound.
	 * - left bound and right bound of every pixel between start and end as left and right bound.
	 * - left bound of the last pixel as left bound, and right bound as the end value (100%).
	 */
	if (startIndexFloor == endIndexFloor) {
		// both are on the same pixel, we can fade it partially. We fade it by 50% since the left and right bounds
		// are 0 and 1.
		float amountToFadePixel = 0.5;

		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * amountToFadePixel
		);
	} else {
		// first pixel calculations
		float firstPixelLeftBound = 1;
		float firstPixelRightBound = smoothingFunction(floor(startIndex + 1), endIndex, startIndex, 1, 0);
		float firstPixelFadeAmount = (firstPixelLeftBound + firstPixelRightBound) / 2;

		// last pixel calculations
		float lastPixelLeftBound = smoothingFunction(floor(endIndex), endIndex, startIndex, 1, 0);
		float lastPixelRightBound = 0;
		float lastPixelFadeAmount = (lastPixelLeftBound + lastPixelRightBound) / 2;

		// set first pixel
		tempData.set(
			startIndexFloor,
			tempData.get(startIndexFloor),
			tempData.getOpacity(startIndexFloor) * firstPixelFadeAmount
		);


		// for every pixel in between, we calculate what the left and right bounds are, and average them.
		// to this, we use a clever trick: we know the left and right pixel values, and the average will be exactly
		// halfway, so we can just do left + 0.5. Then, the calculation will be much faster.
		for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
			float pixelAverageFadeAmount = smoothingFunction(i + 0.5, endIndex, startIndex, 1, 0);

			tempData.set(i, tempData.get(i), tempData.getOpacity(i) * pixelAverageFadeAmount);
		}

		// set last pixel
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

	// Preserves the original behavior exactly: the start is offset by this stage's pipeline position (startIndex), the
	// end is passed as the raw sampled member value (the helpers previously read this->endIndex directly, un-offset).
	if (this->startIndex.value > this->endIndex.value)
		calculateBackwardGradient(startIndex + this->startIndex.value, this->endIndex.value, tempData);
	else
		calculateForwardGradient(startIndex + this->startIndex.value, this->endIndex.value, tempData);

	this->state = this->stage->state;
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string OpacityGradient::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("OpacityGradient");
	// Config emits each endpoint's RECIPE; sampled values are state.
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
