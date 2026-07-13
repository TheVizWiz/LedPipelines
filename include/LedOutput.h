#pragma once

#include "Color.h"

namespace ledpipelines {
	/**
	 * The backend that LedPipelines renders into. This is the library's single seam to the outside world: the core
	 * computes colors into a buffer and hands them to a LedOutput, which is responsible for the actual LEDs (FastLED on
	 * hardware, an ESPHome light, the web previewer, a test double, ...). The core depends on nothing but this
	 * interface, which is what keeps it free of any hardware/driver dependency.
	 *
	 * An output must be registered with ledpipelines::setOutput() before initialize() or run() is called - there is no
	 * default. See outputs/FastLEDOutput.h for the standard hardware backend.
	 */
	struct LedOutput {
		virtual ~LedOutput() = default;

		// Strip topology, read once by TemporaryLedData::initialize() to size the render buffers. A backend with a
		// single strip returns stripCount() == 1.
		virtual int stripCount() const = 0;
		virtual int stripSize(int strip) const = 0;

		// Per-frame lifecycle, called by LedPipelineStage::run(): clear() blanks the backend's buffer before a frame,
		// show() pushes the finished frame to the LEDs.
		virtual void clear() = 0;
		virtual void show() = 0;

		// Write one finished pixel: `color` is the final, opacity-baked RGB for LED `indexInStrip` on strip `strip`.
		// Called by TemporaryLedData::populate() once per visible pixel each frame.
		virtual void setPixel(int strip, int indexInStrip, RGBA color) = 0;
	};

	// Register the backend LedPipelines renders into. REQUIRED before initialize()/run(); passing nullptr clears it.
	// The library does not take ownership - the caller keeps the output alive for as long as pipelines run.
	void setOutput(LedOutput* output);

	// The currently registered output, or nullptr if none has been set. Core code guards on null and no-ops (logging
	// an error) rather than crashing, so a forgotten setOutput() fails loudly but safely.
	LedOutput* getOutput();
} // namespace ledpipelines
