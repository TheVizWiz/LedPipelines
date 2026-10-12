#pragma once

#include "Color.h"
#include "LedOutput.h"

#include "enums/BlendingMode.h"

namespace ledpipelines {
	/**
	 * A per-frame pixel buffer that effects render into in calculate(). Buffers are meant to be short lived and
	 * span every LED across all strips of the registered output.
	 */
	class TemporaryLedData {
	private:
		/** Logical start index of each strip, for set(stripIndex, ledIndex, ...). */
		static int* startIndexes;

	public:
		/**
		 * Raw, unchecked pixel buffer indexed by PHYSICAL index (bufferSize wide); `.a` is opacity. Acquired from a
		 * pool on construction and returned on destruction.
		 */
		RGBA* data;

		/**
		 * True once any pixel has been written, including writes into the off-strip padding.
		 */
		bool anyAreModified = false;

		/**
		 * Total LEDs across all strips (the logical length). Effects address logical 0..size and may also write into
		 * [-padding, size+padding) without being clipped. Set in initialize().
		 */
		static int size;

		/**
		 * Off-strip headroom on EACH side of the visible strip (currently == size). Logical index L maps to physical
		 * index L + padding. Lets moving / repeating effects render off-strip and shift the data back into view later;
		 * only logical 0..size is sent to the output. Set in initialize().
		 */
		static int padding;

		/**
		 * size + 2*padding. Whole-buffer operations (clear, merge, shift, full-strip effects) should sweep
		 * 0..bufferSize in PHYSICAL coordinates so the padding stays consistent with the visible region.
		 */
		static int bufferSize;

		/**
		 * Must be called after setOutput() and before any pipeline is run. Drops all pooled buffers.
		 */
		static void initialize();

		/**
		 * Proxy returned by operator[]: reads go through get(), writes go through set() using the color's own alpha
		 * as the opacity.
		 */
		struct PixelRef {
			TemporaryLedData& owner;
			int index;

			operator RGBA() const {
				return owner.get(index);
			}

			PixelRef& operator=(RGBA color) {
				owner.set(index, color, color.a);
				return *this;
			}

			// Copies the referenced pixel, not the proxy.
			PixelRef& operator=(const PixelRef& other) {
				RGBA color = other.owner.get(other.index);
				owner.set(index, color, color.a);
				return *this;
			}
		};

		/**
		 * Bounds-checked access by LOGICAL index (negative or >= size addresses the padding). For raw physical
		 * access use `data` directly.
		 */
		PixelRef operator[](int index) {
			return PixelRef{*this, index};
		}

		TemporaryLedData(RGBA color = RGBA::Black);

		~TemporaryLedData();

		// Non-copyable: copies would alias the pooled buffer and double-release it.
		TemporaryLedData(const TemporaryLedData&) = delete;

		TemporaryLedData& operator=(const TemporaryLedData&) = delete;

		// The moved-from instance releases nothing on destruction.
		TemporaryLedData(TemporaryLedData&& other) noexcept;

		void merge(TemporaryLedData& other, BlendingMode blendingMode);

		/**
		 * Whether any pixel in the VISIBLE strip (logical 0..size) has non-zero opacity. Unlike anyAreModified, this
		 * ignores the off-strip padding.
		 */
		bool hasVisiblePixels() const;

		/**
		 * Reset every pixel (including padding) to `color` at zero opacity, as if freshly constructed.
		 */
		void clear(RGBA color = RGBA::Black);

		/**
		 * Return a copy shifted by `offset` pixels (positive toward higher indices). Fractional offsets spread a pixel
		 * across two neighbors at proportional opacity, keeping its color. Pixels shifted past the padded buffer's
		 * ends are dropped.
		 */
		TemporaryLedData shift(float offset) const;

		// Write the visible pixels to the backend with opacity premultiplied into RGB.
		void populate(LedOutput& output) const;

		void set(int index, RGBA color, uint8_t opacity = UINT8_MAX);

		void set(int stripIndex, int ledIndex, RGBA& color, uint8_t opacity = UINT8_MAX);

		RGBA get(int index) const;

		uint8_t getOpacity(int index) const;
	};
}
