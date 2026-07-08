#pragma once

#include "FastLED.h"

#include "enums/BlendingMode.h"

namespace ledpipelines {
	/**
	 * A class representing a buffer of data used to build out effects. Each effect populates
	 * a buffer passed to it in its calculate() method. Buffers are meant to be short lived.
	 * They hold enough information for every LED in FastLED, including multiple strips.
	 */
	class TemporaryLedData {
	private:
		/**
		 * Store the start indices so they can be recalled when setting in 2D matrix form (e.g. setting pixels on
		 * specific strips).
		 */
		static int* startIndexes;

	public:
		/**
		 * The internal data stored in the temporary buffer. This is heap memory created when the buffer
		 * is instantiated, and deleted afterwards.
		 */
		CRGB* data;

		/**
		 * To tell in the merging algorithm whether to merge the current pixel, we need to know if it was used
		 * at all. We can do this using another array that stores the opacity value.
		 */
		uint8_t* opacity;

		/**
		 * Tracks if any LEDs have been set. Default to false, changes to true when a call to set() results in an LED
		 * being set.
		 */
		bool anyAreModified = false;

		/**
		 *The total number of LEDs added to FastLED (the logical strip length). This is populated in the initialize()
		 * method. Effects address pixels in logical coordinates 0..size (and, thanks to the padding below, may also
		 * write slightly off-strip into [-padding, size+padding) without their data being clipped).
		 */
		static int size;

		/**
		 * Off-strip headroom on EACH side of the visible strip, in pixels (currently == size). The backing buffer is
		 * bufferSize = size + 2*padding wide, and a logical index L maps to physical index L + padding. This lets
		 * moving / repeating / shifting effects render into negative or past-the-end coordinates without losing pixels:
		 * the data survives in the padding and can be shifted back into view. Only the visible middle (logical 0..size)
		 * is sent to FastLED. Set in initialize().
		 */
		static int padding;

		/**
		 * The physical width of the backing buffers: size + 2*padding. Whole-buffer operations (clear, merge, shift,
		 * and full-strip effects like FadeIn/FadeOut) sweep 0..bufferSize in PHYSICAL coordinates so the padding
		 * regions stay consistent with the visible region. Set in initialize().
		 */
		static int bufferSize;

		/**
		 * Initialize static information needed for all LED data. This should be called
		 * before any calls to pipelines (run, calculate, or anything else.).
		 */
		static void initialize();


		/**
		 * Access a pixel by LOGICAL index (0..size for on-strip pixels; negative or >=size addresses the padding). The
		 * logical index is offset by `padding` to reach the physical buffer slot. No bounds checking - callers that may
		 * go out of the padded range should use set()/get(), which clamp.
		 * @param index the logical index in the temporary data to access / write to.
		 * @return the CRGB data living at that (offset) index in the temporary buffer.
		 */
		CRGB& operator[](int index) const {
			return data[index + padding];
		}

		/**
		 * Constructor for creating LED data. Will create a buffer dynamically based on how many LEDs are added in
		 */
		TemporaryLedData(CRGB color = CRGB::Black);

		~TemporaryLedData();

		// Each instance uniquely owns its backing buffers (returned to the pool on destruction). Copying would alias
		// those buffers and double-release them into the pool, so copying is forbidden.
		TemporaryLedData(const TemporaryLedData&) = delete;

		TemporaryLedData& operator=(const TemporaryLedData&) = delete;

		// Movable so buffers can be returned by value (e.g. from shift()) without copying or double-releasing to the
		// pool: the moved-from instance relinquishes ownership and releases nothing on destruction.
		TemporaryLedData(TemporaryLedData&& other) noexcept;

		void merge(TemporaryLedData& other, BlendingMode blendingMode);

		/**
		 * Whether any pixel in the VISIBLE strip (logical 0..size) has non-zero opacity. Distinct from anyAreModified,
		 * which is set for writes anywhere in the padded buffer - a copy can light only the off-strip padding yet still
		 * have anyAreModified true. Tiling loops (e.g. Repeat) use this to stop once shifted copies leave the screen.
		 */
		bool hasVisiblePixels() const;

		/**
		 * Reset every pixel back to `color` at zero opacity, as if freshly constructed. Used to discard a buffer's
		 * contents for reuse within a frame without releasing and reacquiring it from the pool.
		 */
		void clear(CRGB color = CRGB::Black);

		/**
		 * Shift the contents of this buffer by `offset` pixels (positive shifts toward higher indices, negative toward
		 * lower) and return the result as a new buffer, leaving this one untouched. Fractional offsets are resampled
		 * so a lit pixel that lands between two output pixels spreads across both at proportional opacity, keeping the
		 * color and conserving total light - the same "partial coverage scales opacity" convention used elsewhere
		 * (e.g. SolidSegment, OpacityGradient). Pixels shifted off either end are dropped.
		 */
		TemporaryLedData shift(float offset) const;

		void populateFastLed() const;

		void printData() const;

		void set(int index, CRGB color, uint8_t opacity = UINT8_MAX);

		void set(int stripIndex, int ledIndex, CRGB& color, uint8_t opacity = UINT8_MAX);

		CRGB get(int index) const;

		uint8_t getOpacity(int index) const;
	};
} // namespace ledpipelines
