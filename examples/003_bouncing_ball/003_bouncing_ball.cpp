
#include "Arduino.h"
#include "LedPipelines.h"
#include "outputs/FastLEDOutput.h"

#define LED_PIN 5


using namespace ledpipelines;
using namespace ledpipelines::effects;


/**
 * 003: Bouncing Ball
 *
 * A color-changing "ball" that falls down the strip, bounces, and decelerates back up to its starting height, forever.
 * Covers moving segments, animating colors, eased motion, and combining motions into a bounce.
 */

CRGB leds[100];
FastLEDOutput output;

LedPipelineStage* pipeline;


void setup() {
	FastLED.addLeds<WS2812B, LED_PIN, GBR>(leds, 100);

	setOutput(&output);
	initialize();

	/**
	 * LedPipelines are composable, so we build the effect in parts. The simplest ball is a single-color SolidSegment.
	 */

	SolidSegment::Builder(CRGB::White, 10);

	/**
	 * HSVGradient interpolates from its startGradient to its endGradient over time. With only a start, it is static.
	 */

	HSVGradient::Builder(0, 9).startGradient(HSVA(0, 1, 1), HSVA(360, 1, 1));

	/**
	 * This gradient is 10 units long (0-9). HSV 0 to 360 is one full turn of the hue wheel, giving a rainbow segment.
	 */
	HSVGradient::Builder(0, 9)
		.runtimeMs(1000)
		.startGradient(HSVA(0, 1, 1), HSVA(360, 1, 1))
		.endGradient(HSVA(360, 1, 1), HSVA(720, 1, 1));

	/**
	 * Shifting both ends by 360 makes the rainbow appear to flow through the segment. Once an effect has a runtime it
	 * only runs once, so we wrap it in a Loop.
	 */

	auto ball = HSVGradient::Builder(0, 9)
					.runtimeMs(1000)
					.startGradient(HSVA(0, 1, 1), HSVA(360, 1, 1))
					.endGradient(HSVA(360, 1, 1), HSVA(720, 1, 1))
					.wrap(Loop::Builder());

	SolidSegment::Builder(CRGB::White, 10).wrap(Moving::Builder(1000).startPosition(0).endPosition(80));

	/**
	 * Moving moves its inner effect from startPosition to endPosition over the given ms. Positions are relative: a
	 * segment already offset by 10 would move from 10 to 90. smoothingFunction sets how it moves; INVERSE_QUADRATIC
	 * decelerates, like a ball rising (the start of the strip is the bottom of the bounce).
	 */

	SolidSegment::Builder(CRGB::White, 10)
		.wrap(Moving::Builder(1000).startPosition(0).endPosition(80).smoothingFunction(
			SmoothingFunction::INVERSE_QUADRATIC));

	/**
	 * A second Moving brings the ball back down. Moves add up and the ball is already at 80, so this one goes from 0
	 * to -80. Anchoring effects around 0 keeps this math easy.
	 */

	SolidSegment::Builder(CRGB::White, 10)
		.wrap(Moving::Builder(1000).startPosition(0).endPosition(80).smoothingFunction(
			SmoothingFunction::INVERSE_QUADRATIC))
		.wrap(Moving::Builder(1000)
				  .startPosition(0)
				  .endPosition(-80)
				  .smoothingFunction(SmoothingFunction::QUADRATIC)
				  .delayMs(1000))
		.timebox(2000)
		.loop();

	/**
	 * delayMs(1000) holds the second Moving until the first finishes; a finished Moving stays at its endPosition. The
	 * SolidSegment never completes, so timebox() (shorthand for wrap(TimeBox::Builder())) ends the effect after one
	 * bounce and loop() repeats it. QUADRATIC falls (slow to fast); INVERSE_QUADRATIC rises (fast to slow).
	 */

	ball.wrap(Moving::Builder(1000).startPosition(0).endPosition(80).smoothingFunction(
				  SmoothingFunction::INVERSE_QUADRATIC))
		.wrap(Moving::Builder(1000)
				  .startPosition(0)
				  .endPosition(-80)
				  .smoothingFunction(SmoothingFunction::QUADRATIC)
				  .delayMs(1000))
		.timebox(2000)
		.loop();

	/**
	 * The TimeBox resets its inner effects all the way down the tree on every bounce, restarting the ball's colors.
	 * block() adds a ResetBlocker, which ignores resets unless the inner effect is Done - and the looping ball never is.
	 * Since everything is just a Builder passed to wrap(), the motions can be saved as separate variables.
	 */

	auto upstroke =
		Moving::Builder(2000).startPosition(0).endPosition(80).smoothingFunction(SmoothingFunction::INVERSE_QUADRATIC);

	auto downstroke = Moving::Builder(2000)
						  .startPosition(0)
						  .endPosition(-80)
						  .smoothingFunction(SmoothingFunction::INVERSE_QUADRATIC)
						  .delayMs(2000);


	pipeline = ball.block().wrap(upstroke).wrap(downstroke).timebox(2000).loop().build();
}


void loop() {
	pipeline->run();
}
