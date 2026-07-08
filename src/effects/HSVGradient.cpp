#include "effects/HSVGradient.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

namespace {
	// Component-wise linear interpolation between two FHSV values. Hue is interpolated on its raw value (no wheel
	// wrapping) so multi-turn sweeps like 0..720 are preserved; wrapping happens only at conversion in fhsvToRgb.
	FHSV lerpFhsv(const FHSV& a, const FHSV& b, float t) {
		return FHSV{a.h + (b.h - a.h) * t, a.s + (b.s - a.s) * t, a.v + (b.v - a.v) * t};
	}
} // namespace

HSVGradient::HSVGradient(
	float startPosition,
	float endPosition,
	unsigned long runtimeMs,
	FHSV startGradientStart,
	FHSV startGradientEnd,
	FHSV endGradientStart,
	FHSV endGradientEnd,
	SmoothingFunction smoothingFunction,
	uint8_t opacity,
	BlendingMode blendingMode
)
	: LedPipelineStage(blendingMode),
	  TimedEffect(runtimeMs),
	  startPosition(startPosition),
	  endPosition(endPosition),
	  startGradientStart(startGradientStart),
	  startGradientEnd(startGradientEnd),
	  endGradientStart(endGradientStart),
	  endGradientEnd(endGradientEnd),
	  smoothingFunction(smoothingFunction),
	  opacity(opacity) {}

void HSVGradient::calculate(float startIndex, TemporaryLedData& tempData) {
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
	if (runtimeMs == 0) {
		timeT = 0;
	} else {
		this->elapsedPercentage = (float)elapsedMs() / (float)runtimeMs;
		float clamped = elapsedPercentage < 0 ? 0 : (elapsedPercentage > 1 ? 1 : elapsedPercentage);
		timeT = smoothingFunction(clamped);
	}

	// Collapse the two time-endpoint gradients into the single gradient in effect right now.
	const FHSV nowStart = lerpFhsv(startGradientStart, endGradientStart, timeT);
	const FHSV nowEnd = lerpFhsv(startGradientEnd, endGradientEnd, timeT);

	// POSITION axis. Endpoints are relative to where this stage sits in the pipeline (startIndex), matching
	// SolidSegment. Normalize so lo < hi regardless of the order given, swapping the colors alongside the positions so
	// the gradient stays anchored to its endpoints.
	float lo = startIndex + startPosition;
	float hi = startIndex + endPosition;
	FHSV loColor = nowStart;
	FHSV hiColor = nowEnd;
	if (lo > hi) {
		float tmpPos = lo;
		lo = hi;
		hi = tmpPos;
		FHSV tmpColor = loColor;
		loColor = hiColor;
		hiColor = tmpColor;
	}

	const float span = hi - lo;

	const int loFloor = (int)lo;
	const int hiFloor = (int)hi;

	// The FHSV at a position along the segment, by fractional distance. A zero-length span collapses to the low color.
	auto colorAt = [&](float pos) -> CRGB {
		float posT = span > 0 ? (pos - lo) / span : 0.0f;
		if (posT < 0) posT = 0;
		if (posT > 1) posT = 1;
		return fhsvToRgb(lerpFhsv(loColor, hiColor, posT));
	};

	if (loFloor == hiFloor) {
		// Whole segment sits within one pixel: light it partially by the segment length, colored at the segment start.
		tempData.set(loFloor, colorAt(lo) * span, opacity);
	} else {
		// First (possibly partial) pixel: color at the segment start, opacity scaled by how much of the pixel it
		// covers.
		const float firstPixelCoverage = 1 - (lo - loFloor);
		if (firstPixelCoverage != 0) {
			tempData.set(loFloor, colorAt(lo), opacity * firstPixelCoverage);
		}

		// Full interior pixels: color sampled at the pixel center (i + 0.5) so the gradient is evaluated mid-pixel.
		for (int i = loFloor + 1; i < hiFloor; i++) {
			tempData.set(i, colorAt(i + 0.5f), opacity);
		}

		// Last (possibly partial) pixel.
		const float lastPixelCoverage = hi - hiFloor;
		if (lastPixelCoverage != 0) {
			tempData.set(hiFloor, colorAt(hi), opacity * lastPixelCoverage);
		}
	}

	// Hold on endGradient once the timeline completes (only meaningful for an animated gradient).
	if (runtimeMs != 0 && elapsedPercentage >= 1) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
	}
}

void HSVGradient::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
}
