#include "Arduino.h"
#include "LedPipelines.h"

#define LED_PIN 5 // the pin we're attaching our LEDs to.



// these just make it easier to make effects without having to write out the entire thing every time.
// e.g. now we can do SolidEffect instead of ledpipelines::effects::SolidEffect. totally optional.
using namespace ledpipelines;
using namespace ledpipelines::effects;

/**
 * 002: Cycle RGB Full Strip.
 * NOTE: if you haven't yet seen example 001 (basic full color), check that example out first.
 *
 * WHAT WE'RE BUILDING:
 * We want to build an effect that just cycles the entire strip between red, green, and blue infinitely. So the strip
 * will turn red for 2 seconds, then green for two seconds, then blue for two seconds, and then back to red.
 *
 * That means we'll need to learn:
 * - how to make an effect only last for a specific amount of time
 * - how to add effects and make them play one after another
 * - how to make effects loop over and over (either a set number of times, or infinitely).
 */

/**
 * LedPipelines depends on FastLED. To use FastLED, first declare all the LEDs that you would like to use. In this case,
 * we are declaring an array of 100  LEDs.
 */
CRGB leds[100];

/**
 * This object will store our LedPipeline once we initialize it.
 */
BaseLedPipelineStage *pipeline;


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
     * Let's take a second before we actually create the pipelines to think about what we want to do. We want to create an
     * effect that will cycle through red for 2 seconds, then green for 2 seconds, then blue for 2 seconds, and then
     * repeat indefinitely.
     *
     * In 001, we learned how to make an effect that lights up the whole strip in a single color. Since we need to do that
     * three times, we'll need three of those effects. We'll also need some way to make the effect only play for a
     * certain amount of time (in this case, 2 seconds each). In 001, we saw that a solid effect will continue to play
     * indefinitely, which won't work here. We also need a way to play effects in sequence, one after another. Finally,
     * we also need a way to loop effects over and over.
     *
     *
     * Let's tackle the first problem first - limiting effects to 2 seconds each. To do this, let's introduce the idea
     * of a WrapperEffect. A WrapperEffect is an effect that wraps around another effect, and applies some modifier to it.
     * For example, there is a WrapperEffect called OpacityGradient that gradually fades in the effect that's passed to it
     * over a given number of LEDs. LedPipelines comes with a library of WrapperEffects. In this case, we can make use of
     * the TimeBoxedEffect, which sets the max duration of the internal effect to be a certain number of seconds.
     */

    auto redEffect = new SolidEffect({CRGB::Red});
    auto redEffectLimited = new TimeBoxedEffect(redEffect, {2});


    /**
     * The two lines above first set up a red effect, then wrap it in a TimeBoxedEffect that limits it to run for 2
     * seconds. If we were to add this to the pipeline like we did last time, we would get a red light for two seconds,
     * and then nothing until we reset the microcontroller.
     *
     * Let's set up the blue and green effects. I'll do these on one line, just to show what that looks like.
     */
    auto greenEffectLimited = new TimeBoxedEffect(new SolidEffect({CRGB::Green}), {2});
    auto blueEffectLimited = new TimeBoxedEffect(new SolidEffect({CRGB::Blue}), {2});


    /**
     * Now that we have all of our effects, we can think about the next part - making them play one at a time, and then
     * loop back around. When we set up more complicated effects like this, the order that we set up our pipelines matters
     * for what the final effect will look like. To make this effect, we'll need to use two new things: a SeriesLedPipeline,
     * and a LoopEffect.
     *
     * The SeriesLedPipeline will run all of its stages, one at a time. It'll run each stage until the stage says that
     * it's done. In this case, if we add a TimeBoxedEffect, it'll run until the TimeBoxedEffect finishes, then move on
     * to the next stage, and so on.
     *
     * The LoopEffect is another WrapperEffect that will run the effect that it wraps until the internal effect finishes,
     * and then run it again. And again. It'll run a specified number of times, or it'll run it until an indefinite number
     * of times if no number is specified.
     *
     *
     * Here, we have a choice - we could:
     * 1. add all the R/G/B TimeBoxedEffects to the SeriesLedPipeline, and then wrap it in a LoopEffect.
     * 2. wrap each R/G/B TimeBoxedEffect in a LoopEffect, and then add each LoopEffect to a SeriesLedPipeline.
     *
     * If we think about the control flow of option 2, it'll try to run the first stage in the pipeline, which would
     * loop the TimeBoxed red effect until it finishes, and then loop over it again, and again, and again. That's not
     * what we want. We want to play all three R/G/B effects, and then loop all of them. So we want option 1. Here's
     * what that looks like:
     */

    pipeline = new LoopEffect(
            /**
             * inside the loop effect, we can supply the pipeline we talked about before. We could set up another variable
             * for the internal pipeline, and then add each stage separately, but the addStage method returns a reference
             * to the pipeline itself so that we can chain multiple calls.
             */
            (new SeriesLedPipeline) // yes, the parentheses here are required :(
                    ->addStage(redEffectLimited)
                    ->addStage(greenEffectLimited)
                    ->addStage(blueEffectLimited)
    );


    /**
     * Let's take a second and talk about why we used a SeriesLedPipeline above. There are two kinds of LedPipelines:
     * SeriesLedPipelines and ParallelLedPipelines. a SeriesLedPipeline will run the first effect until it completes,
     * then run the next effect, and so on and so forth. A ParallelLedPipeline will run all of its stages at the same time,
     * layering them one on top of another in the order that they were added to the pipeline. The layering takes into
     * account the opacity of each stage/effect/layer and composites them the same way Photoshop would.
     *
     * If we used parallel instead of series above, then it would have tried to run all R+G+B stages at the same time,
     * and since all of them are at full opacity, we would have only seen the last effect added (which was the blue effect).
     * You can try this for yourself and see if we get what we expect, by just changing from SeriesLedPipeline to
     * ParallelLedPipeline above!
     */
}


void loop() {
    /**
     * Just like in 001, we can use the run command to run the pipeline. Now, we should get looping RGB effects, over
     * and over again.
     */
    pipeline->run();
}
