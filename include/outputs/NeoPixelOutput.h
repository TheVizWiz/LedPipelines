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
//     output.begin();
//     ledpipelines::setOutput(&output);
//     ledpipelines::initialize();
//
// This backend drives a SINGLE strip (stripCount() == 1), since one Adafruit_NeoPixel object is one strip. That covers
// the common case; multiple strips would need one NeoPixelOutput per strip and a small multiplexing output on top.

#include <Adafruit_NeoPixel.h>

#include "../LedColor.h"
#include "../LedOutput.h"

namespace ledpipelines {
	// Owns one Adafruit_NeoPixel strip. Call begin() once, then register via setOutput().
	struct NeoPixelOutput : LedOutput {
		Adafruit_NeoPixel strip;

		// Same arguments as Adafruit_NeoPixel. `type` (NEO_* color-order + speed flags) makes Adafruit_NeoPixel handle the
		// strand's byte order internally.
		NeoPixelOutput(uint16_t count, int16_t pin, neoPixelType type = NEO_GRB + NEO_KHZ800)
			: strip(count, pin, type) {}

		// Initializes the underlying strip and blanks it. Call once before rendering.
		void begin() {
			strip.begin();
			strip.clear();
			strip.show();
		}

		int stripCount() const override {
			return 1;
		}

		int stripSize(int /*strip*/) const override {
			return strip.numPixels();
		}

		void clear() override {
			strip.clear();
		}

		void show() override {
			strip.show();
		}

		void setPixel(int /*strip*/, int indexInStrip, RGBA color) override {
			// `color` is already opacity-baked by populate(), so alpha is dropped.
			this->strip.setPixelColor(indexInStrip, color.r, color.g, color.b);
		}
	};
}
