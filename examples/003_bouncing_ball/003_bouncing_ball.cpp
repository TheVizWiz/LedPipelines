
#include "Arduino.h"
#include "LedPipelines.h"

#define LED_PIN 5 // the pin we're attaching our LEDs to.


// these just make it easier to make effects without having to write out the entire thing every time.
// e.g. now we can do Solid instead of ledpipelines::effects::Solid. totally optional.
using namespace ledpipelines;
using namespace ledpipelines::effects;


/**
 * 003: Bouncing Ball
 * Note: If you haven't yet seen example 002 (rgb cycle), check that example out first.
 *
 * WHAT WE'RE BUILDING:
 * We want to build an effect that looks like a bouncing ball. The ball should "fall" down the strip, gaining velocity,
 * until it bounces against the end of the strip and bounces back to the top, losing velocity until it is back at the
 * height that it started at. It should repeat this forever. The ball is going to change colors over time.
 *
 * That means we'll need to lear:
 * - how to make a moving segment of LEDs
 * - how to make a segment of LEDs change colors over time
 * - how to make that move "accelerate" over time
 * - how to repeat that acceleration in the opposite direction so the ball bounces
 * - how to repeat indefinitely
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
	 * Let's build this pipeline in stages. LedPipelines are composable, so we can build effects in parts and then put
	 * them together after. The first part is the ball itself. The simplest way to do this would just be to set up
	 * a segment of LEDs that are the same colors. To do this, we can simply use the SolidSegment:
	 *
	 */

	SolidSegment::Builder(CRGB::White, 10);

	/**
	 * However, we said that we want the ball to change colors over time. To do this, we can use the HSVGradient source.
	 * the HSVGradient lets you specify two gradients: a startGradient and an endGradient. Over time, it will
	 * interpolate between the start and end gradients. If you just specify a start, it will be a static gradient. Let's
	 * look at an example
	 */

	HSVGradient::Builder(0, 9).startGradient(FHSV(0, 1, 1), FHSV(360, 1, 1));

	/**
	 * This gradient is 10 units long (0-9 in the constructor). It starts at HSV 0-1-1 (red), and ends at HSV 360-1-1
	 * (also red, but 1 turn [360 degrees] around the circle). This gives us a rainbow segment 10 units long. However,
	 * we want to animate this rainbow segment. To do this, let's use the endGradient.
	 */
	HSVGradient::Builder(0, 9)
		.runtimeMs(1000)
		.startGradient(FHSV(0, 1, 1), FHSV(360, 1, 1))
		.endGradient(FHSV(360, 1, 1), FHSV(720, 1, 1));

	/**
	 * Since we go from 0-360 on the start and 360-720 at the end, this essentially makes it look like the rainbow is
	 * moving through the segment. This is a pretty common effect, so you'll see it in lots of places. However, now that
	 * the effect has a time element, it only runs one time. To make it run over and over, let's wrap it in a Loop.
	 *
	 * That's everything we need for the "ball", so let's save it as a ball!
	 */

	auto ball = HSVGradient::Builder(0, 9)
					.runtimeMs(1000)
					.startGradient(FHSV(0, 1, 1), FHSV(360, 1, 1))
					.endGradient(FHSV(360, 1, 1), FHSV(720, 1, 1))
					.wrap(Loop::Builder());

	/**
	 * Now let's talk about getting the ball to Move. To do this, we can use the Moving wrapper, which moves its
	 * internal effect with the specified parameters.
	 */

	SolidSegment::Builder(CRGB::White, 10).wrap(Moving::Builder(1000).startPosition(0).endPosition(80));

	/**
	 * This will move the internal effect (the SolidSegment) from 0 to 80 over 1 second (1000 ms). Note that the Moving
	 * effect, as well as all other positional effects, are relative. If the Solid segment started out offset by 10
	 * pixels, it will end up moving from 10 to 90 instead of 0 to 80.
	 *
	 * The Moving wrapper also allows us to specify *how* it moves - linearly, smoothly, etc. Since we want it to move
	 * like it is decelerating - remember, the start of the strip is the bottom of the bounce - we can use the inverse
	 * quadratic motion.
	 */

	SolidSegment::Builder(CRGB::White, 10)
		.wrap(Moving::Builder(1000).startPosition(0).endPosition(80).smoothingFunction(
			SmoothingFunction::INVERSE_QUADRATIC));

	/**
	 * We want this motion to repeat, but the other way, to simulate the ball bouncing back down from its peak.
	 * To do this, we can wrap the motion in another Moving call. This time, we can move from 80 back down to 0.
	 * However, this time, the ball is already at 80. Therefore, we should move it from 0 down to -80. Remember, all
	 * effects are relative. It's good practice to anchor them around 0 so it's easy to do math with them.
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
	 * You might notice we added a few extra pieces here to make it all work together.
	 *
	 * 1. the second Moving has a delayMs(1000). This tells it to only start the move after 1 second, allowing time for
	 * the first move (from bounce to the apex) to complete before the second one starts. Remember, Moving effects have
	 * three parts: before, during, and after. Before the effect begins, it has no effect. During, it transitions from
	 * startPosition to endPosition for runtimeMs milliseconds. After, it stays at the end position until the effect
	 * it is wrapping finishes.
	 * 2. We timebox both moving effects. timebox() is shorthand for wrap(TimeBox::Builder()). Some wrapper effects are
	 * used so often they get their own wrapper function shorthands. You'll see timebox(), loop(), shift(), and block(),
	 * among others. We timebox to 2 seconds to give time for the first move, then second move, to complete. We have to
	 * timebox since the innermost effect, the SolidSegment, never completes. If we don't timebox it, it will never
	 * stop, even after both moves complete. Timeboxing allows us to force the effect to finish after "one full bounce".
	 * 3. Finally, we wrap the whole thing in a loop. Since the timebox limits it to one full bounce, we have to loop it
	 * to get more than one bounce.
	 * 4. Notice the different smoothing functions we use: we use Quadratic when going from apex to the bottom of the
	 * bounce, and Inverse Quadratic going from the bottom back to the apex. Quadratic starts moving slow and ends fast;
	 * Inverse starts fast and ends slow. We know gravity is quadratic so we use these two. There are a bunch of other
	 * smoothing functions we can use: Linear, Sine, Smooth Linear, etc. Feel free to play around with them!
	 *
	 *
	 * Now, we can put everything together by adding the ball where it has to go:
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
	 * One last piece - you might notice that the "ball" animation resets every single time it bounces, instead of
	 * continuing. This is due to the TimeBox resetting the inner effects, all the way down the tree. To stop this from
	 * happening, we can add in a ResetBlocker. A ResetBlocker stops resets from resetting the inner effect, unless the
	 * inner effect is also in its Done state. Since the ball colors loop forever, it never hits Done, so the resets
	 * will never propagate down the tree to the ball itself. We can organize this a little better by breaking
	 * the builders into separate pieces. Remember, everything is just a Builder that gets passed to wrap(), so
	 * we can save those as separate variables.
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
	/**
	 * Just like in 001, we can use the run command to run the pipeline. Now, we should get looping bouncing ball, over
	 * and over again. The ball should have a rainbow effect inside it, flowing towards the origin of the LED strip.
	 * Try playing around with the effect and seeing if you can make the entire strip a single color that shifts over
	 * time instead of a gradient that shifts over time!
	 */
	pipeline->run();
}
