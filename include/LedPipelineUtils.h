#pragma once

#include "FastLED.h"
#include "enums/BlendingMode.h"
#include "enums/SmoothingFunction.h"
#include "enums/SamplingFunction.h"
#include "TemporaryLedData.h"
#include "LedPipelinesLogger.h"


/**
 * A floating-point HSV color. Unlike FastLED's uint8_t CHSV (hue 0-255, artistic rainbow map), this keeps full
 * precision so hues can be interpolated smoothly and wrap cleanly. hue is in degrees and may be any value - it is
 * wrapped into [0, 360) at conversion time (e.g. 500 wraps to 140, -30 wraps to 330). s and v are in [0, 1].
 */
struct FHSV {
	float h = 0;
	float s = 1;
	float v = 1;

	FHSV() = default;

	FHSV(float h, float s, float v) : h(h), s(s), v(v) {}
};

/**
 * Convert a floating-point HSV color to CRGB. Hue is wrapped into [0, 360); saturation and value are clamped to [0, 1].
 */
CRGB fhsvToRgb(FHSV hsv);

CRGB operator*(CRGB first, CRGB second);

CRGB &operator*=(CRGB &first, const CRGB &second);

CRGB operator*(CRGB first, float amount);

CRGB &operator*=(CRGB &first, float amount);

namespace ledpipelines {
	extern uint64_t minMicrosBetweenUpdates;

	/**
	 * Set the max refresh rate of LedPipelines. Defaults to no max refresh rate. Note that this is NOT blocking in the same
	 * way that FastLED.show() is blocking; if you call pipeline.run() more often than the max refresh rate, the extra calls
	 * will just be ignored until the next update is ready. An update is not *guaranteed* to take place at this interval,
	 * but it is guaranteed that it will take at *least* this interval, even if you try to update LedPipelines faster than this.
	 * @param refreshesPerSecond the new max refresh rate, in refreshes / second. e.g. 30fps, 50fps, 144fps.
	 */
	void setMaxRefreshRate(float refreshesPerSecond);


	/**
	 * Convert a Color and opacity to a hex code, in form FFFFFFFF, or RGBA.
	 * @param color the color to convert, in RGB form.
	 * @param opacity the opacity of the color to convert.
	 * @return the color in Hex form in a string.
	 */
	String colorToHex(CRGB color, uint8_t opacity);
}
