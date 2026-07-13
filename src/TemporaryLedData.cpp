#include "TemporaryLedData.h"
#include "LedOutput.h"  // getOutput() - topology comes from the registered backend

#include <algorithm>  // std::min
#include <cmath>      // floorf
#include <vector>

using namespace ledpipelines;


int TemporaryLedData::size = 0;

int TemporaryLedData::padding = 0;

int TemporaryLedData::bufferSize = 0;

int* TemporaryLedData::startIndexes = nullptr;

namespace {
	/**
	 * A buffer pool for the backing array of TemporaryLedData. Effects allocate a fresh buffer every frame (often many
	 * per frame), which on a microcontroller causes heap fragmentation and timing jitter. Instead of new[]/delete[] on
	 * every construction, we hand out preallocated arrays from this free list and return them on destruction.
	 *
	 * The pool is self-sizing: it grows to the high-water mark (the deepest pipeline tree ever rendered) over the first
	 * few frames, then never allocates again in steady state. Buffers are only freed when the pool is reset in
	 * initialize(), so on an embedded device memory usage is deterministic.
	 *
	 * Each buffer is a single RGBA array: color and opacity (RGBA::a) live together, so there is no parallel opacity
	 * array to keep in lockstep.
	 */
	std::vector<RGBA*> bufferPool;

	RGBA* acquireBuffer() {
		if (!bufferPool.empty()) {
			RGBA* buffer = bufferPool.back();
			bufferPool.pop_back();
			return buffer;
		}
		return new RGBA[TemporaryLedData::bufferSize];
	}

	void releaseBuffer(RGBA* buffer) {
		bufferPool.push_back(buffer);
	}
} // namespace

void TemporaryLedData::initialize() {
	// Topology comes from the registered output backend (FastLED, ESPHome, viewer, ...). setOutput() is required
	// before initialize(); without it there are no strips to measure, so bail loudly and leave size == 0 (run() also
	// guards on a null output, so nothing renders until one is registered).
	LedOutput* output = getOutput();
	if (output == nullptr) {
		return;
	}

	// calculate the total number of LEDs.
	TemporaryLedData::startIndexes = new int[output->stripCount()];
	size = 0;
	for (int i = 0; i < output->stripCount(); i++) {
		TemporaryLedData::startIndexes[i] = size;
		size += output->stripSize(i);
	}

	// One strip-width of off-strip headroom on each side, so moving/repeating effects can render past the strip edges
	// without losing pixels (see the header). The backing buffer is therefore three strip-widths wide.
	padding = size;
	bufferSize = size + 2 * padding;

	// Drop any pooled buffers: they were sized for a previous `size` and would be too small if re-initialized.
	for (auto& buffer : bufferPool) {
		delete[] buffer;
	}
	bufferPool.clear();
}

TemporaryLedData::TemporaryLedData(RGBA color) {
	data = acquireBuffer();
	// Buffers are reused, so they carry stale data from a previous owner. Clear to defaults so each effect starts with
	// its own clean buffer and cannot bleed into another effect's pixels.
	clear(color);
}

void TemporaryLedData::clear(RGBA color) {
	// Physical sweep over the whole padded buffer so the off-strip padding is cleared too (a stale value left in the
	// padding could otherwise be shifted into view by a later shift()/Repeat). Opacity 0 = unlit.
	color.a = 0;
	for (int i = 0; i < bufferSize; i++) {
		data[i] = color;
	}
	anyAreModified = false;
}


TemporaryLedData::TemporaryLedData(TemporaryLedData&& other) noexcept
	: data(other.data), anyAreModified(other.anyAreModified) {
	// Take ownership of the buffer and null out the source so its destructor releases nothing.
	other.data = nullptr;
}


TemporaryLedData::~TemporaryLedData() {
	// A moved-from instance owns no buffer; nothing to return to the pool.
	if (data != nullptr) releaseBuffer(data);
}

TemporaryLedData TemporaryLedData::shift(float offset) const {
	TemporaryLedData result = TemporaryLedData();

	// A shift by `offset` maps source pixel i to i + offset. Split into a whole-pixel move and a fractional
	// remainder, then work per output pixel: output j draws from exactly two source pixels - (j - whole) with weight
	// (1 - frac) and (j - whole - 1) with weight frac. Each source's light contribution is opacity * weight; the
	// output color is those two colors averaged by their light contribution (so equal colors pass through unchanged
	// and partial coverage dims via opacity rather than tinting toward black), and the output opacity is their sum.
	int whole = (int)floorf(offset);
	float frac = offset - whole;

	// Work in PHYSICAL buffer coordinates across the whole padded buffer, reading/writing the raw array directly.
	// Because the source's off-strip data lives in the padding, a shift that brings a padded pixel into the visible
	// window (e.g. a Repeat copy wrapping in from the left) carries the full, un-clipped pixel with it.
	for (int j = 0; j < bufferSize; j++) {
		int nearIndex = j - whole;
		int farIndex = j - whole - 1;

		float nearLight = (nearIndex >= 0 && nearIndex < bufferSize) ? this->data[nearIndex].a * (1 - frac) : 0;
		float farLight = (frac > 0 && farIndex >= 0 && farIndex < bufferSize) ? this->data[farIndex].a * frac : 0;

		float totalLight = nearLight + farLight;
		if (totalLight == 0) continue;

		RGBA nearColor = (nearLight > 0) ? this->data[nearIndex] : RGBA(RGBA::Black);
		RGBA farColor = (farLight > 0) ? this->data[farIndex] : RGBA(RGBA::Black);

		// Average the two colors weighted by their light share. Weights are normalized to [0, 1] and summed per
		// channel in float so intermediate products never overflow a channel.
		float nearWeight = nearLight / totalLight;
		float farWeight = farLight / totalLight;
		RGBA blended(
			nearColor.r * nearWeight + farColor.r * farWeight,
			nearColor.g * nearWeight + farColor.g * farWeight,
			nearColor.b * nearWeight + farColor.b * farWeight
		);
		blended.a = (uint8_t)std::min(totalLight, (float)UINT8_MAX);

		result.data[j] = blended;
		result.anyAreModified = true;
	}

	return result;
}

void TemporaryLedData::merge(TemporaryLedData& other, BlendingMode blendingMode) {
	// Physical sweep over the whole padded buffer: merging in the padding too keeps off-strip data consistent so a
	// later shift() can bring correctly-merged pixels into view. Each pixel's opacity is its RGBA::a channel.
	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		auto A_alpha = this->data[i].a;
		auto A_rgb = this->data[i];
		auto B_alpha = other.data[i].a;
		auto B_rgb = other.data[i];
		// if other pixel has no opacity, we skip this pixel. We can't skip it in MASK mode, because in MASK mode
		// opacity of 0 pixels are masked out.
		if (!B_alpha && blendingMode != BlendingMode::MASK) continue;

		this->anyAreModified = true;
		switch (blendingMode) {
		case BlendingMode::OVERWRITE:
			this->data[i] = B_rgb;
			this->data[i].a = B_alpha;
			break;
		case BlendingMode::ADD:
			this->data[i] = A_rgb + B_rgb;
			this->data[i].a = std::min(A_alpha + B_alpha, (int)UINT8_MAX);
			break;
		case BlendingMode::MULTIPLY:
			this->data[i] *= A_rgb * B_rgb;
			this->data[i].a = (B_alpha * B_alpha) / 255;
			break;
		case BlendingMode::NORMAL:
			this->data[i].r = ((255 - B_alpha) * A_rgb.r + B_alpha * B_rgb.r) / 255;
			this->data[i].g = ((255 - B_alpha) * A_rgb.g + B_alpha * B_rgb.g) / 255;
			this->data[i].b = ((255 - B_alpha) * A_rgb.b + B_alpha * B_rgb.b) / 255;
			this->data[i].a = B_alpha + ((255 - B_alpha) * A_alpha) / 255;
			break;
		case BlendingMode::MASK:
			// in mask mode, let through everywhere that has 100% opacity
			// and nothing through where the mask has 0% opacity.
			this->data[i] = (A_rgb * B_rgb);
			this->data[i].a = (A_alpha * B_alpha) / 255;
			break;
		}
	}
}

bool TemporaryLedData::hasVisiblePixels() const {
	// Scan only the visible middle third (logical 0..size -> physical padding..padding+size).
	for (int i = padding; i < padding + size; i++) {
		if (data[i].a) return true;
	}
	return false;
}

void TemporaryLedData::set(int index, RGBA color, uint8_t opacity) {
	// `index` is a LOGICAL coordinate. Accept the padded range [-padding, size+padding) so an effect can write
	// slightly off-strip (its pixels live in the padding rather than being discarded); reject anything beyond that.
	if (index < -padding || index >= size + padding) return;
	int physical = index + padding;
	color.a = opacity;
	this->data[physical] = color;
	anyAreModified = true;
}

void TemporaryLedData::set(int stripIndex, int ledIndex, RGBA& color, uint8_t opacity) {
	LedOutput* output = getOutput();
	if (output == nullptr || stripIndex < 0 || stripIndex >= output->stripCount()) return; // strip doesn't exist
	int index = startIndexes[stripIndex] + ledIndex; // index in array
	this->set(index, color, opacity);
}

RGBA TemporaryLedData::get(int index) const {
	// LOGICAL index; accept the padded range and offset into the physical buffer.
	if (index < -padding || index >= size + padding) return RGBA(RGBA::Black);

	return this->data[index + padding];
}

uint8_t TemporaryLedData::getOpacity(int index) const {
	if (index < -padding || index >= size + padding) return 0;

	return this->data[index + padding].a;
}

void TemporaryLedData::populate(LedOutput& output) const {
	int currentLed = 0;
	for (int i = 0; i < output.stripCount(); i++) {
		for (int j = 0; j < output.stripSize(i); (j++, currentLed++)) {
			RGBA pixel = this->get(currentLed);
			uint8_t opacity = pixel.a;
			output.setPixel(
				i,
				j,
				RGBA(
					(pixel.red * opacity / 255),
					(pixel.green * opacity / 255),
					(pixel.blue * opacity / 255)
				)
			);
		}
	}
}
