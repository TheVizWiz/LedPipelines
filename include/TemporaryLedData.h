#pragma once

#include "Color.h"
#include "LedOutput.h"

#include "enums/BlendingMode.h"

namespace ledpipelines {
	/**
	 * A class representing a buffer of data used to build out effects. Each effect populates
	 * a buffer passed to it in its calculate() method. Buffers are meant to be short lived.
	 * They hold enough information for every LED across all strips of the registered output.
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
		 * The internal pixel buffer: one RGBA per slot, where `.a` is the pixel's opacity (0 = unlit/transparent, 255 =
		 * fully opaque). Color and opacity live together in the single value rather than in a parallel array. Heap
		 * memory acquired from the pool when the buffer is constructed and returned on destruction.
		 */
		RGBA* data;

		/**
		 * Tracks if any LEDs have been set. Default to false, changes to true when a call to set() results in an LED
		 * being set.
		 */
		bool anyAreModified = false;

		/**
		 *The total number of LEDs across the registered output's strips (the logical strip length). Populated in initialize()
		 * method. Effects address pixels in logical coordinates 0..size (and, thanks to the padding below, may also
		 * write slightly off-strip into [-padding, size+padding) without their data being clipped).
		 */
		static int size;

		/**
		 * Off-strip headroom on EACH side of the visible strip, in pixels (currently == size). The backing buffer is
		 * bufferSize = size + 2*padding wide, and a logical index L maps to physical index L + padding. This lets
		 * moving / repeating / shifting effects render into negative or past-the-end coordinates without losing pixels:
		 * the data survives in the padding and can be shifted back into view. Only the visible middle (logical 0..size)
		 * is sent to the output. Set in initialize().
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
		 * Proxy returned by operator[] so that both reading and writing a pixel by bracket go through the same
		 * bounds-checked path as get()/set() - rather than exposing the raw buffer slot. Assigning an RGBA
		 * (`data[i] = color`) writes the color INCLUDING its opacity (color.a): `data[i] = RGBA(r, g, b, a)` sets
		 * opacity a, and `data[i] = RGBA::Red` (whose alpha defaults to 255) is fully opaque. It clamps the index and
		 * marks the buffer modified, exactly like set(). Reading (`RGBA c = data[i]`) is exactly `get(i)`.
		 */
		struct PixelRef {
			TemporaryLedData& owner;
			int index;

			// Read: RGBA c = data[i];  -> get(i) (clamped; returns Black off-range).
			operator RGBA() const {
				return owner.get(index);
			}

			// Write a color, honoring its own alpha as the pixel's opacity. Routes through set() (bounds check + modified
			// flag) passing color.a so the folded opacity is respected rather than overwritten.
			PixelRef& operator=(RGBA color) {
				owner.set(index, color, color.a);
				return *this;
			}

			// Chained assignment (data[i] = data[j] = color) copies the resolved color (with its alpha), not the proxy.
			PixelRef& operator=(const PixelRef& other) {
				RGBA color = other.owner.get(other.index);
				owner.set(index, color, color.a);
				return *this;
			}
		};

		/**
		 * Access a pixel by LOGICAL index (0..size for on-strip pixels; negative or >=size addresses the padding).
		 * Returns a PixelRef proxy so the access is bounds-checked and opacity-aware in both directions (see PixelRef):
		 * `data[i] = color` lights the pixel opaquely, `RGBA c = data[i]` reads it. For raw, unchecked slot access use
		 * the `data` array directly.
		 */
		PixelRef operator[](int index) {
			return PixelRef{*this, index};
		}

		/**
		 * Constructor for creating LED data. Will create a buffer dynamically based on how many LEDs are added in
		 */
		TemporaryLedData(RGBA color = RGBA::Black);

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
		void clear(RGBA color = RGBA::Black);

		/**
		 * Shift the contents of this buffer by `offset` pixels (positive shifts toward higher indices, negative toward
		 * lower) and return the result as a new buffer, leaving this one untouched. Fractional offsets are resampled
		 * so a lit pixel that lands between two output pixels spreads across both at proportional opacity, keeping the
		 * color and conserving total light - the same "partial coverage scales opacity" convention used elsewhere
		 * (e.g. SolidSegment, OpacityGradient). Pixels shifted off either end are dropped.
		 */
		TemporaryLedData shift(float offset) const;

		// Write the finished, opacity-baked frame to the given backend: one setPixel() per visible LED, preserving
		// per-strip grouping (topology comes from the same LedOutput). Called by run() after all effects have rendered.
		void populate(LedOutput& output) const;

		void set(int index, RGBA color, uint8_t opacity = UINT8_MAX);

		void set(int stripIndex, int ledIndex, RGBA& color, uint8_t opacity = UINT8_MAX);

		RGBA get(int index) const;

		uint8_t getOpacity(int index) const;
	};
} // namespace ledpipelines
