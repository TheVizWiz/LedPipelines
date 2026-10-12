#include "Arduino.h"
#include "LedPipelines.h"
#include "outputs/FastLEDOutput.h"

#define LED_PIN 5


using namespace ledpipelines;
using namespace ledpipelines::effects;

/**
 * 002: Cycle RGB Full Strip.
 *
 * Cycles the entire strip between red, green, and blue for 2 seconds each, forever. Covers limiting an effect's
 * duration, playing effects one after another, and looping them.
 */

CRGB leds[100];
FastLEDOutput output;

LedPipelineStage *pipeline;


void setup() {
	FastLED.addLeds<WS2812B, LED_PIN, GBR>(leds, 100);

	setOutput(&output);
	initialize();

	/**
	 * A WrapperEffect wraps another effect and modifies it; TimeBox limits the inner effect to a max duration in ms.
	 * wrap() on the inner effect's Builder returns the wrapper's Builder, so wraps can be chained and build() called
	 * once at the very end.
	 */
	auto redEffectLimited = Solid::Builder(CRGB::Red).wrap(TimeBox::Builder(2000));

	auto greenEffectLimited = Solid::Builder(CRGB::Green).wrap(TimeBox::Builder(2000));
	auto blueEffectLimited = Solid::Builder(CRGB::Blue).wrap(TimeBox::Builder(2000));

	/**
	 * A SeriesLedPipeline runs its stages one at a time, moving on when each finishes. Loop re-runs its inner effect
	 * whenever it finishes, forever unless a count is given. Order matters: we loop the whole series, since wrapping
	 * each TimeBox in its own Loop would repeat red forever and never reach green.
	 */

	pipeline =
		SeriesLedPipeline::Builder()
			.addStage(redEffectLimited)
			.addStage(greenEffectLimited)
			.addStage(blueEffectLimited)
			.wrap(Loop::Builder())
			.build();

	/**
	 * A ParallelLedPipeline would instead run all its stages at once, layering them by opacity in the order they were
	 * added - here only the last (blue) would show. Try swapping it in above to see.
	 */
}


void loop() {
	pipeline->run();
}
