#pragma once

// The Adafruit NeoPixel backend for LedPipelines. Like FastLEDOutput, this is an opt-in header: it is the only place
// that includes Adafruit_NeoPixel, is NOT pulled in by LedPipelines.h, and compiles because the sketch's own project
// supplies the Adafruit NeoPixel library (an example dependency, not a library dependency).
//
// Unlike FastLED - whose data pin is a compile-time template parameter - Adafruit_NeoPixel takes the pin and count as
// RUNTIME constructor arguments. So this backend needs no template pin: pin, count, and color order are plain values,
// which makes it the natural choice when those must be configurable at runtime (e.g. driven from ESPHome YAML).
//
// Usage:
//     #include "LedPipelines.h"
//     #include "outputs/NeoPixelOutput.h"
//     ...
//     static ledpipelines::NeoPixelOutput output(COUNT, PIN, NEO_GRB + NEO_KHZ800);
//     output.begin();                 // inits the underlying strip (calls Adafruit_NeoPixel::begin)
//     ledpipelines::setOutput(&output);
//     ledpipelines::initialize();     // reads topology (the count) from the output
//
// This backend drives a SINGLE strip (stripCount() == 1), since one Adafruit_NeoPixel object is one strip. That covers
// the common case; multiple strips would need one NeoPixelOutput per strip and a small multiplexing output on top.

#include <Adafruit_NeoPixel.h>

#include "../Color.h"
#include "../LedOutput.h"

namespace ledpipelines {
	// Bridges LedPipelines to one Adafruit_NeoPixel strip, which this object owns. Construct with the same
	// (count, pin, type) you would pass Adafruit_NeoPixel, call begin() once, then register via setOutput().
	struct NeoPixelOutput : LedOutput {
		Adafruit_NeoPixel strip;

		// count: number of LEDs. pin: data pin. type: NEO_* color-order + speed flags (e.g. NEO_GRB + NEO_KHZ800).
		// The type flag makes Adafruit_NeoPixel handle the strand's byte order internally, so setPixel() below always
		// passes plain r, g, b.
		NeoPixelOutput(uint16_t count, int16_t pin, neoPixelType type = NEO_GRB + NEO_KHZ800)
			: strip(count, pin, type) {}

		// Initialize the underlying strip (allocates its buffer and configures the pin). Call once before rendering,
		// after construction. Provided as a passthrough so callers do not need to touch the wrapped object.
		void begin() {
			strip.begin();
			strip.clear();
			strip.show();  // start dark
		}

		int stripCount() const override {
			return 1;
		}

		int stripSize(int /*strip*/) const override {
			return strip.numPixels();
		}

		void clear() override {
			// Buffer-only clear (Adafruit_NeoPixel::clear does not touch the LEDs until show()), matching LedOutput's
			// contract that clear() blanks the pending frame and show() latches it.
			strip.clear();
		}

		void show() override {
			strip.show();
		}

		void setPixel(int /*strip*/, int indexInStrip, RGBA color) override {
			// Always r, g, b - Adafruit_NeoPixel remaps to the strand's native order via the type flag. `color` is
			// already opacity-baked by populate(); alpha is not sent to the LED (physical LEDs have no alpha).
			strip.setPixelColor(indexInStrip, color.r, color.g, color.b);
		}
	};
} // namespace ledpipelines
