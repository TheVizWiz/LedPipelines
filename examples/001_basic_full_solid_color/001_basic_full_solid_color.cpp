#include "Arduino.h"
#include "LedPipelines.h"
#include "outputs/FastLEDOutput.h"

#define LED_PIN 5


/**
 * 001 - Full Solid Color.
 *
 * Lights up the entire strip in a single color. Covers creating a pipeline, adding an effect to it, and running it.
 */

/**
 * The core library is hardware-independent and renders through a pluggable output. This sketch uses the FastLED
 * backend (outputs/FastLEDOutput.h), so the LEDs are declared and added to FastLED as usual.
 */
CRGB leds[100];
ledpipelines::FastLEDOutput output;

/**
 * Every object in LedPipelines derives from the abstract LedPipelineStage, so any pipeline can be stored through a
 * LedPipelineStage pointer.
 */
ledpipelines::LedPipelineStage *pipeline;


void setup() {
	FastLED.addLeds<WS2812B, LED_PIN, GBR>(leds, 100);

	/**
	 * Register the output with setOutput() before calling initialize(): initialize() reads the LED layout from it and
	 * allocates memory for the effects. It can be called before or after creating your pipelines.
	 */
	ledpipelines::setOutput(&output);
	ledpipelines::initialize();

	/**
	 * Effects are built from stages. A single effect like Solid is a stage on its own; pipelines such as
	 * SeriesLedPipeline and ParallelLedPipeline combine multiple stages (see 002). Every effect is created through its
	 * Builder, and build() produces the actual stage.
	 */
	pipeline = ledpipelines::effects::Solid::Builder(CRGB::Red).build();
}


void loop() {
	/**
	 * run() is blocking and also calls show() on the registered output. Call it as often as possible.
	 */
	pipeline->run();
}
