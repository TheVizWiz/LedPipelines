#pragma once

// The FastLED backend for LedPipelines. This is the ONLY header in the library that includes FastLED, and it is NOT
// pulled in by LedPipelines.h. Hardware sketches include it explicitly:
//
//     #include "LedPipelines.h"
//     #include "outputs/FastLEDOutput.h"
//     ...
//     FastLED.addLeds<WS2812B, PIN, GRB>(leds, COUNT);
//     static ledpipelines::FastLEDOutput output;
//     ledpipelines::setOutput(&output);
//     ledpipelines::initialize();                        // reads topology from FastLED
//
// The sketch's own project must supply FastLED (it is an example dependency, not a library dependency).

#include "FastLED.h"

#include "../Color.h"
#include "../LedOutput.h"

namespace ledpipelines {
	// Bridges LedPipelines to FastLED: topology and pixel output come straight from the global FastLED controller that
	// the sketch already set up with addLeds<>(). Register one instance via setOutput() before initialize().
	struct FastLEDOutput : LedOutput {
		int stripCount() const override {
			return FastLED.count();
		}

		int stripSize(int strip) const override {
			return FastLED[strip].size();
		}

		void clear() override {
			FastLED.clear();
		}

		void show() override {
			FastLED.show();
		}

		void setPixel(int strip, int indexInStrip, RGBA color) override {
			// `color` is already opacity-baked by populate(), so alpha is dropped.
			FastLED[strip][indexInStrip] = ::CRGB(color.r, color.g, color.b);
		}
	};
}
