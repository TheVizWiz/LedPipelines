
#include "Arduino.h"
#include "LedPipelines.h"
#include "outputs/FastLEDOutput.h"

#define LED_PIN 5


using namespace ledpipelines;
using namespace ledpipelines::effects;


/**
 * 004: Bouncing Ball
 *
 * The same bouncing ball as 003 (see that first), built a simpler way. Introduces shared effects, which break the tree
 * structure of effects and unlock graphs: what they are, their footguns, and how they simplify deeply nested wraps.
 */

CRGB leds[100];
FastLEDOutput output;

LedPipelineStage* pipeline;


void setup() {
	FastLED.addLeds<WS2812B, LED_PIN, GBR>(leds, 100);

	setOutput(&output);
	initialize();

	auto ball_003 = HSVGradient::Builder(0, 9)
						.runtimeMs(1000)
						.startGradient(HSVA(0, 1, 1), HSVA(360, 1, 1))
						.endGradient(HSVA(360, 1, 1), HSVA(720, 1, 1))
						.wrap(Loop::Builder());

	auto upstroke_003 =
		Moving::Builder(2000).startPosition(0).endPosition(80).smoothingFunction(SmoothingFunction::INVERSE_QUADRATIC);

	auto downstroke_003 = Moving::Builder(2000)
							  .startPosition(0)
							  .endPosition(-80)
							  .smoothingFunction(SmoothingFunction::INVERSE_QUADRATIC)
							  .delayMs(2000);


	auto pipeline_003 = ball_003.block().wrap(upstroke_003).wrap(downstroke_003).timebox(2000).loop().build();

	delete pipeline_003;


	/**
	 * Nested Movings add up: the downstroke shifts whatever the upstroke already produced, so it has to go 0->-80 to
	 * cancel the +80, and that math compounds with every extra Moving. Instead, wrap the ball in each Moving separately
	 * and run the two in a SeriesLedPipeline, so they simply go 0->80 and 80->0.
	 */

	auto ball = HSVGradient::Builder(0, 9)
					.runtimeMs(10000)
					.startGradient(HSVA(0, 1, 1), HSVA(0, 1, 1))
					.endGradient(HSVA(360, 1, 1), HSVA(360, 1, 1))
					.wrap(Loop::Builder());

	auto upstroke = ball.wrap(Moving::Builder(2000).startPosition(0).endPosition(80).smoothingFunction(
								  SmoothingFunction::INVERSE_QUADRATIC))
						.timebox(2000);

	auto downstroke = ball.wrap(Moving::Builder(2000).startPosition(80).endPosition(0).smoothingFunction(
									SmoothingFunction::QUADRATIC))
						  .timebox(2000);

	pipeline = SeriesLedPipeline::Builder().addStage(upstroke).addStage(downstroke).loop().build();


	delete pipeline;

	/**
	 * This is easier to follow, but the ball's color resets on every stroke. Each ball.wrap() copies the builder, so the
	 * upstroke and downstroke build two separate balls on independent timers. block() can't fix this - it only stops a
	 * parent's reset() from cascading down; it can't merge two instances into one. Effects normally form a tree, where
	 * no effect can be used in two places.
	 *
	 * The Shared wrapper wraps a completely built effect instead of a builder, so every use points at the same instance,
	 * turning the tree into a graph. We still block() before sharing so resets from the Moving effects don't reach the
	 * colored segment.
	 */

	auto sharedBall = HSVGradient::Builder(0, 9)
						  .runtimeMs(10000)
						  .startGradient(HSVA(0, 1, 1), HSVA(0, 1, 1))
						  .endGradient(HSVA(360, 1, 1), HSVA(360, 1, 1))
						  .loop()
						  .block()
						  .shared();

	upstroke = sharedBall
				   .wrap(Moving::Builder(2000).startPosition(0).endPosition(80).smoothingFunction(
					   SmoothingFunction::INVERSE_QUADRATIC))
				   .timebox(2000);

	downstroke = sharedBall
					 .wrap(Moving::Builder(2000).startPosition(80).endPosition(0).smoothingFunction(
						 SmoothingFunction::QUADRATIC))
					 .timebox(2000);

	pipeline = SeriesLedPipeline::Builder().addStage(upstroke).addStage(downstroke).loop().build();


}


void loop() {
	pipeline->run();
}
