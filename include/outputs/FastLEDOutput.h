#pragma once

// The FastLED backend for LedPipelines. This is the ONLY header in the library that includes FastLED, and it is NOT
// pulled in by LedPipelines.h - the core never references it. Hardware sketches include it explicitly:
//
//     #include "LedPipelines.h"
//     #include "outputs/FastLEDOutput.h"
//     ...
//     FastLED.addLeds<WS2812B, PIN, GRB>(leds, COUNT);   // register strips with FastLED as usual
//     static ledpipelines::FastLEDOutput output;         // then register the backend
//     ledpipelines::setOutput(&output);
//     ledpipelines::initialize();                        // reads topology from the output (i.e. from FastLED)
//
// It compiles because the sketch's own project supplies FastLED (see library.json: FastLED is an example dependency,
// not a library dependency). LedPipelines' own CRGB (Color.h) is layout-compatible with FastLED's, so the pixel copy
// in setPixel is a plain field assignment.

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
			// LedPipelines' RGBA -> FastLED's CRGB. Distinct types (that's what avoids the global-name collision), so
			// copy the RGB channels explicitly. `color` is already opacity-baked by populate(); alpha is not sent to
			// the LED (physical LEDs have no alpha).
			FastLED[strip][indexInStrip] = ::CRGB(color.r, color.g, color.b);
		}
	};
} // namespace ledpipelines
