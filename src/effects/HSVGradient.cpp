#include "effects/HSVGradient.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

namespace {
	// Hue is interpolated on its raw value (no wheel wrapping) so multi-turn sweeps like 0..720 are preserved; wrapping
	// happens only at conversion in HSVA::toRGBA.
	HSVA lerpFhsv(const HSVA& a, const HSVA& b, float t) {
		return HSVA{
			a.h + (b.h - a.h) * t,
			a.s + (b.s - a.s) * t,
			a.v + (b.v - a.v) * t,
			(uint8_t)(a.a + (b.a - a.a) * t + 0.5f)
		};
	}
}

HSVGradient::HSVGradient(
	Value<float> startPosition,
	Value<float> endPosition,
	Value<unsigned long> runtimeMs,
	Value<HSVA> startGradientStart,
	Value<HSVA> startGradientEnd,
	Value<HSVA> endGradientStart,
	Value<HSVA> endGradientEnd,
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
	this->startPosition.sample();
	this->endPosition.sample();
	this->startGradientStart.sample();
	this->startGradientEnd.sample();
	this->endGradientStart.sample();
	this->endGradientEnd.sample();
	this->smoothingFunction.sample();
}

void HSVGradient::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->startTimeMs = millis();
		this->state = LedPipelineRunningState::RUNNING;
	}

	// runtimeMs == 0 means static: pinned to startGradient forever, never advances or reports DONE.
	float timeT;
	if (runtimeMs.value == 0) {
		timeT = 0;
	} else {
		this->elapsedPercentage = (float)elapsedMs() / (float)runtimeMs.value;
		float clamped = elapsedPercentage < 0 ? 0 : (elapsedPercentage > 1 ? 1 : elapsedPercentage);
		timeT = smoothingFunction.value(clamped);
	}

	const HSVA nowStart = lerpFhsv(startGradientStart.value, endGradientStart.value, timeT);
	const HSVA nowEnd = lerpFhsv(startGradientEnd.value, endGradientEnd.value, timeT);

	// Swap the colors alongside the positions so the gradient stays anchored to its endpoints.
	float lo = startIndex + startPosition.value;
	float hi = startIndex + endPosition.value;
	HSVA loColor = nowStart;
	HSVA hiColor = nowEnd;
	if (lo > hi) {
		float tmpPos = lo;
		lo = hi;
		hi = tmpPos;
		HSVA tmpColor = loColor;
		loColor = hiColor;
		hiColor = tmpColor;
	}

	const float span = hi - lo;

	const int loFloor = (int)lo;
	const int hiFloor = (int)hi;

	auto colorAt = [&](float pos) -> RGBA {
		float posT = span > 0 ? (pos - lo) / span : 0.0f;
		if (posT < 0) posT = 0;
		if (posT > 1) posT = 1;
		return lerpFhsv(loColor, hiColor, posT).toRGBA();
	};

	if (loFloor == hiFloor) {
		const RGBA c = colorAt(lo);
		tempData.set(loFloor, c * span, c.a);
	} else {
		const float firstPixelCoverage = 1 - (lo - loFloor);
		if (firstPixelCoverage != 0) {
			const RGBA c = colorAt(lo);
			tempData.set(loFloor, c, c.a * firstPixelCoverage);
		}

		for (int i = loFloor + 1; i < hiFloor; i++) {
			const RGBA c = colorAt(i + 0.5f);
			tempData.set(i, c, c.a);
		}

		const float lastPixelCoverage = hi - hiFloor;
		if (lastPixelCoverage != 0) {
			const RGBA c = colorAt(hi);
			tempData.set(hiFloor, c, c.a * lastPixelCoverage);
		}
	}

	if (runtimeMs.value != 0 && elapsedPercentage >= 1) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
	}
}

void HSVGradient::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
	startPosition.sample();
	endPosition.sample();
	startGradientStart.sample();
	startGradientEnd.sample();
	endGradientStart.sample();
	endGradientEnd.sample();
	smoothingFunction.sample();
}

#ifdef LEDPIPELINES_SERIALIZATION
std::string HSVGradient::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("HSVGradient");
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
			jsonField("endGradientEndSampled", endGradientEnd.value));
	}
	result += "}";
	return result;
}
#endif
