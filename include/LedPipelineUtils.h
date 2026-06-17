#pragma once

#include "FastLED.h"
#include "enums/BlendingMode.h"
#include "enums/SmoothingFunction.h"
#include "enums/SamplingFunction.h"
#include "TemporaryLedData.h"
#include "LedPipelinesLogger.h"


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
