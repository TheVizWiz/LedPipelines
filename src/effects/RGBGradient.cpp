#include "effects/RGBGradient.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

namespace {
	// Per-channel linear interpolation between two RGBA values, INCLUDING alpha. Rounds to the nearest byte per
	// channel. Interpolating alpha lets a gradient fade opacity across its corners (e.g. opaque -> transparent).
	RGBA lerpRgb(const RGBA& a, const RGBA& b, float t) {
		return RGBA(
			(uint8_t)(a.r + (b.r - a.r) * t + 0.5f),
			(uint8_t)(a.g + (b.g - a.g) * t + 0.5f),
			(uint8_t)(a.b + (b.b - a.b) * t + 0.5f),
			(uint8_t)(a.a + (b.a - a.a) * t + 0.5f)
		);
	}
} // namespace

RGBGradient::RGBGradient(
	Value<float> startPosition,
	Value<float> endPosition,
	Value<unsigned long> runtimeMs,
	Value<RGBA> startGradientStart,
	Value<RGBA> startGradientEnd,
	Value<RGBA> endGradientStart,
	Value<RGBA> endGradientEnd,
	Value<SmoothingFunction> smoothingFunction,
	BlendingMode blendingMode
)
	: LedPipelineStage(blendingMode),
	  TimedEffect(std::move(runtimeMs)),
	  startPosition(std::move(startPosition)),
	  endPosition(std::move(endPosition)),
	  startGradientStart(std::move(startGradientStart)),
	  startGradientEnd(std::move(startGradientEnd)),
	  endGradientStart(std::move(endGradientStart)),
	  endGradientEnd(std::move(endGradientEnd)),
	  smoothingFunction(std::move(smoothingFunction)) {
	// Sample every input once for this lifecycle (constants always yield the same value; random ones draw one). Each
	// Value caches its own draw in .value.
	this->startPosition.sample();
	this->endPosition.sample();
	this->startGradientStart.sample();
	this->startGradientEnd.sample();
	this->endGradientStart.sample();
	this->endGradientEnd.sample();
	this->smoothingFunction.sample();
}

void RGBGradient::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	// TIME axis. runtimeMs == 0 (the default when no time is given) means the gradient is static: it stays pinned to
	// startGradient forever (timeT = 0, so endGradient is intentionally never reached) and never advances or reports
	// DONE. Otherwise elapsedMs() drives progress from startGradient (timeT=0) to endGradient (timeT=1) through the
	// smoothing function, clamped and held at 1 past the end. elapsedMs() also holds at 0 during any lead-in delay, so
	// a delayed animated gradient sits on startGradient until the delay elapses.
	float timeT;
	if (runtimeMs.value == 0) {
		timeT = 0;
	} else {
		this->elapsedPercentage = (float)elapsedMs() / (float)runtimeMs.value;
		float clamped = elapsedPercentage < 0 ? 0 : (elapsedPercentage > 1 ? 1 : elapsedPercentage);
		timeT = smoothingFunction.value(clamped);
	}

	// Collapse the two time-endpoint gradients into the single gradient in effect right now. Uses the SAMPLED corner
	// colors (each Value caches its own draw, re-drawn on reset()).
	const RGBA nowStart = lerpRgb(startGradientStart.value, endGradientStart.value, timeT);
	const RGBA nowEnd = lerpRgb(startGradientEnd.value, endGradientEnd.value, timeT);

	// POSITION axis. Endpoints are relative to where this stage sits in the pipeline (startIndex), matching
	// SolidSegment. Normalize so lo < hi regardless of the order given, swapping the colors alongside the positions so
	// the gradient stays anchored to its endpoints.
	float lo = startIndex + startPosition.value;
	float hi = startIndex + endPosition.value;
	RGBA loColor = nowStart;
	RGBA hiColor = nowEnd;
	if (lo > hi) {
		float tmpPos = lo;
		lo = hi;
		hi = tmpPos;
		RGBA tmpColor = loColor;
		loColor = hiColor;
		hiColor = tmpColor;
	}

	const float span = hi - lo;

	const int loFloor = (int)lo;
	const int hiFloor = (int)hi;

	// The RGBA at a position along the segment, by fractional distance. A zero-length span collapses to the low color.
	auto colorAt = [&](float pos) -> RGBA {
		float posT = span > 0 ? (pos - lo) / span : 0.0f;
		if (posT < 0) posT = 0;
		if (posT > 1) posT = 1;
		return lerpRgb(loColor, hiColor, posT);
	};

	// Opacity is the interpolated color's own alpha (colorAt(...).a); partial end pixels scale it by their coverage.
	if (loFloor == hiFloor) {
		// Whole segment sits within one pixel: light it partially by the segment length, colored at the segment start.
		const RGBA c = colorAt(lo);
		tempData.set(loFloor, c * span, c.a);
	} else {
		// First (possibly partial) pixel: color at the segment start, opacity scaled by how much of the pixel it
		// covers.
		const float firstPixelCoverage = 1 - (lo - loFloor);
		if (firstPixelCoverage != 0) {
			const RGBA c = colorAt(lo);
			tempData.set(loFloor, c, c.a * firstPixelCoverage);
		}

		// Full interior pixels: color sampled at the pixel center (i + 0.5) so the gradient is evaluated mid-pixel.
		for (int i = loFloor + 1; i < hiFloor; i++) {
			const RGBA c = colorAt(i + 0.5f);
			tempData.set(i, c, c.a);
		}

		// Last (possibly partial) pixel.
		const float lastPixelCoverage = hi - hiFloor;
		if (lastPixelCoverage != 0) {
			const RGBA c = colorAt(hi);
			tempData.set(hiFloor, c, c.a * lastPixelCoverage);
		}
	}

	// Hold on endGradient once the timeline completes (only meaningful for an animated gradient).
	if (runtimeMs.value != 0 && elapsedPercentage >= 1) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
	}
}

void RGBGradient::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
	// Re-roll every input for the next run. For constants these are no-ops; for RandomColor/RandomRange inputs they
	// draw fresh values - the "sample once per lifecycle, resample on reset" contract (see Solid::reset).
	startPosition.sample();
	endPosition.sample();
	startGradientStart.sample();
	startGradientEnd.sample();
	endGradientStart.sample();
	endGradientEnd.sample();
	smoothingFunction.sample();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string RGBGradient::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("RGBGradient");
	// Config emits each input's RECIPE; state (below) emits the sampled values.
	result += jsonRawField("startPosition", startPosition.describeValue());
	result += jsonRawField("endPosition", endPosition.describeValue());
	result += jsonRawField("startGradientStart", startGradientStart.describeValue());
	result += jsonRawField("startGradientEnd", startGradientEnd.describeValue());
	result += jsonRawField("endGradientStart", endGradientStart.describeValue());
	result += jsonRawField("endGradientEnd", endGradientEnd.describeValue());
	result += jsonRawField("smoothingFunction", smoothingFunction.describeValue());
	result += jsonRawField("runtimeMs", runtimeMs.describeValue());
	result += jsonRawField("delayMs", delayMs.describeValue());
	result += jsonField("terminateOnComplete", terminateOnComplete);
	if (includeState) {
		result += jsonStateObject(
			jsonStateHeader() + jsonTimedEffect() +
			jsonField("startGradientStartSampled", startGradientStart.value) +
			jsonField("startGradientEndSampled", startGradientEnd.value) +
			jsonField("endGradientStartSampled", endGradientStart.value) +
			jsonField("endGradientEndSampled", endGradientEnd.value)
		);
	}
	result += "}";
	return result;
}
#endif
