
#include "Arduino.h"
#include "LedPipelines.h"

#define LED_PIN 5 // the pin we're attaching our LEDs to.


// these just make it easier to make effects without having to write out the entire thing every time.
// e.g. now we can do Solid instead of ledpipelines::effects::Solid. totally optional.
using namespace ledpipelines;
using namespace ledpipelines::effects;


/**
 * 004: Bouncing Ball
 * Note: If you haven't yet seen example 003 (bouncing ball), **make sure to** check that example out first.
 *
 * WHAT WE'RE BUILDING:
 * We want to build an effect that looks like a bouncing ball. The ball should "fall" down the strip, gaining velocity,
 * until it bounces against the end of the strip and bounces back to the top, losing velocity until it is back at the
 * height that it started at. It should repeat this forever. The ball is going to change colors over time.
 *
 * This is different from 003's bouncing ball since we'll explore a possibly simpler and easier to understand way of
 * building the same effect. In doing this, we'll learn about shared effects, which break the tree structure we've been
 * using so far and unlock graphs of effects.
 *
 * That means we'll need to learn:
 * - what a shared effect is and how it breaks the contract that we're used to from all other effects
 * - what are the footguns of a shared effect and how to avoid them
 * - how we can use shared effects to simplify more complicated trees and deeply nested wraps
 */

/**
 * LedPipelines depends on FastLED. To use FastLED, first declare all the LEDs that you would like to use. In this case,
 * we are declaring an array of 100  LEDs.
 */
CRGB leds[100];

/**
 * This object will store our LedPipeline once we initialize it.
 */
LedPipelineStage* pipeline;


void setup() {
	/**
	 * The first step is to add all your LEDs to FastLED. In this case, we have 100 WS2812B leds on pin LED_PIN.
	 */
	FastLED.addLeds<WS2812B, LED_PIN, GBR>(leds, 100);

	/**
	 * Always call initialize() after adding all the LEDs to FastLED. LedPipelines sets things up in the background
	 * (such as counting all the leds and creating memory for the effects to follow) in this function. This can be
	 * called before or after actually creating all your LedPipelines, but has to be called *after* adding all your
	 * LEDs to FastLED.
	 */
	initialize();

	/**
	 * Okay, let's start with the old pipeline, that we had in 003:
	 */

	auto ball_003 = HSVGradient::Builder(0, 9)
						.runtimeMs(1000)
						.startGradient(FHSV(0, 1, 1), FHSV(360, 1, 1))
						.endGradient(FHSV(360, 1, 1), FHSV(720, 1, 1))
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
	 * The first thing to recognize is that this pipeline is a little messy. We have a Moving wrapped inside another
	 * Moving, so their positions **add up**: the outer downstroke shifts whatever the inner upstroke already produced.
	 * The upstroke drives the ball 0->80, and because it holds at 80 once its timer runs out, the downstroke has to go
	 * 0->-80 just to cancel that +80 and bring the ball back to the top. That's why the second offset is -80 instead of
	 * 80->0. If we stack more of these - say, two bounces instead of one - each new Moving has to account for the sum of
	 * all the ones inside it, and the offset math compounds fast.
	 *
	 * How can we get around this problem? One way is to break up the moving effect into two separate parts that
	 * run, one after the other. So the first move goes from 0-80, the second from 80-0. To do this, we take a ball,
	 * wrap it in a Moving one time. Then we wrap it in a moving another time, **separately this time**, and then put
	 * both together in a Series pipeline so they run one after another. This is different from 003 since we're no
	 * longer wrapping the Moves inside each other, which makes a lot more sense (to me at least).
	 *
	 * That might look something like this:
	 */

	auto ball = HSVGradient::Builder(0, 9)
					.runtimeMs(10000)
					.startGradient(FHSV(0, 1, 1), FHSV(0, 1, 1))
					.endGradient(FHSV(360, 1, 1), FHSV(360, 1, 1))
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
	 * We can clearly see that the pipeline is a little easier to follow. The upstroke and downstroke are two separate
	 * entities, and one follows the other. Each of them goes from 0 to 80 or 80 to 0 - we no longer need to do that
	 * offset math that we did last time. Cleaner! just one small problem: if you run that pipeline, you'll notice that
	 * the color of the ball resets every time we start an upstroke or a downstroke. It's tempting to reach for block()
	 * here, but block() solves a different problem: it stops a parent's reset() from cascading down into the ball. It
	 * can't make the two balls into one. The real issue is that the ball is fundamentally two separate effects - one
	 * built for the downstroke, one for the upstroke. They run on independent timers that never line up, so their
	 * colors drift apart. No amount of block()-ing merges two separate instances into a single shared one. If we truly
	 * want one ball that is used in both the downstroke and the upstroke,
	 * we can't wrap the ball twice. Each time we call ball.wrap(), it creates a new copy of ball, with a new builder.
	 * Then, when that builder gets built, it creates a new ball. The "graph" of effects we are creating is essentially
	 * a tree: no effect can point to another effect that is used somewhere else in the tree already.
	 *
	 * There is one way to get around this, however: the Shared wrapper. The Shared wrapper wraps a **completely built**
	 * effect, not a builder like every other wrapper. This means when we call build() on a shared effect, it shares
	 * its inner effect with another effect already using it somewhere in the tree. This is the one effect that turns
	 * our tree into a real graph. If we make our ball shared, we can reuse it multiple times in the tree, and it's the
	 * same exact effect getting used multiple times - not just the builder being reused. Note that we'll still have
	 * to block() before we share, to make sure that resets on the moving effect don't make their way down to the
	 * colored segment.
	 */

	auto sharedBall = HSVGradient::Builder(0, 9)
						  .runtimeMs(10000)
						  .startGradient(FHSV(0, 1, 1), FHSV(0, 1, 1))
						  .endGradient(FHSV(360, 1, 1), FHSV(360, 1, 1))
						  .loop()
						  .block()
						  .shared(); // shorthand for wrapping in Shared::Builder(buildShared()): builds this effect NOW and
									 // hands the one built instance to a Shared, so every use of sharedBall points at that
									 // same instance.

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
	/**
	 * Just like in 001, we can use the run command to run the pipeline. Now, we should get looping bouncing ball, over
	 * and over again. The ball should be a solid color that slowly cycles through the whole hue wheel over time as it
	 * bounces.
	 * Try playing around with the effect and seeing if you can make the entire strip a single color that shifts over
	 * time instead of a gradient that shifts over time!
	 */
	pipeline->run();
}
