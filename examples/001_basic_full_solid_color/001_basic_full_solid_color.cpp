#include "Arduino.h"
#include "LedPipelines.h"

#define LED_PIN 5 // the pin we're attaching our LEDs to.


/**
 * 001 - Full Solid Color.
 *
 * WHAT WE'RE BUILDING:
 * A very simple effect that just lights up the entire strip in a single color
 *
 * The most basic effect you could make is the full solid color effect, where the entire strip is lit up in a single
 * color. To do this, we'll learn:
 * - how to create an Led Pipeline
 * - how to add an effect to a pipeline
 * - how to run a pipeline to make effects show up on your LEDs.
 */

/**
 * LedPipelines depends on FastLED. To use FastLED, first declare all the LEDs that you would like to use. In this case,
 * we are declaring an array of 100 LEDs.
 */
CRGB leds[100];

/**
 * This object will store our pipeline once we initialize it. All objects in LedPipelines derive from the base class of
 * BaseLedPipelineStage, so no matter what our pipeline looks like, we can use BaseLedPipelineStage to store it. Since
 * BaseLedPipelineStage is an abstract class, we need to use a pointer here.
 */
ledpipelines::BaseLedPipelineStage *pipeline;


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
    ledpipelines::initialize();

    /**
     * LedPipelines is set up around the idea of stages, which are building blocks used to make effects. The most basic
     * LedPipelineStage is a single effect that does one thing, like light up the entire strip in a single color. However,
     * many times, you might want to make effects that are made up of multiple other effects internally. To make those kinds
     * of effects, you can use actual pipelines, such as SeriesLedPipeline and ParallelLedPipeline. We'll dive into these
     * in future tutorials, such as 002, which will walk through a SeriesLedPipeline.
     *
     * For now, we just want to use a single effect that sets up the entire strip to run in a single color, so we only need
     * the one effect/stage. In LedPipelines, the effect that lights up the entire strip in a single color is called
     * SolidEffect. We can set the pipeline to be the one stage that we want, by also passing in the color that we want
     * to set it to.
     */
    pipeline = new ledpipelines::effects::SolidEffect({CRGB::Red});
}


void loop() {
    /**
     * To run the effects in your pipeline, simply use the run() method on the pipeline. Call this method as often
     * as possible. It is blocking, and will also call FastLED.show();
     */
    pipeline->run();
}

/**
 * As we can see, setting up single effects is super easy! But that's not where LedPipelines shines. LedPipelines is really
 * meant for situations where we have multiple effects that we want to combine to make complicated, intricate designs
 * on LEDs. We will start doing this in the next tutorial, 002.
 */