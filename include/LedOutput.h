#pragma once

#include "LedColor.h"

namespace ledpipelines {
	/**
	 * The backend that LedPipelines renders into (FastLED on hardware, an ESPHome light, the web previewer, a test
	 * double, ...). The core depends only on this interface.
	 *
	 * An output must be registered with ledpipelines::setOutput() before initialize() or run() is called - there is no
	 * default. See outputs/FastLEDOutput.h for the standard hardware backend.
	 */
	struct LedOutput {
		virtual ~LedOutput() = default;

		// Read once by TemporaryLedData::initialize() to size the render buffers.
		virtual int stripCount() const = 0;
		virtual int stripSize(int strip) const = 0;

		// Called by LedPipelineStage::run() each frame: clear() before rendering, show() after.
		virtual void clear() = 0;
		virtual void show() = 0;

		// `color` is the final, opacity-baked RGB. Called once per visible pixel each frame.
		virtual void setPixel(int strip, int indexInStrip, RGBA color) = 0;
	};

	// REQUIRED before initialize()/run(); passing nullptr clears it. Does not take ownership - the caller keeps the
	// output alive for as long as pipelines run.
	void setOutput(LedOutput* output);

	// Returns nullptr if none has been set; core code then no-ops (logging an error) rather than crashing.
	LedOutput* getOutput();
}
