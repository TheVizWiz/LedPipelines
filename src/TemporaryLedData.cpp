#include "TemporaryLedData.h"
#include "LedPipelineUtils.h"

#include <vector>

using namespace ledpipelines;


int TemporaryLedData::size = 0;

int* TemporaryLedData::startIndexes = nullptr;

namespace {
	/**
	 * A buffer pool for the backing arrays of TemporaryLedData. Effects allocate a fresh buffer every frame (often
	 * many per frame), which on a microcontroller causes heap fragmentation and timing jitter. Instead of new[]/delete[]
	 * on every construction, we hand out preallocated array-pairs from this free list and return them on destruction.
	 *
	 * The pool is self-sizing: it grows to the high-water mark (the deepest pipeline tree ever rendered) over the first
	 * few frames, then never allocates again in steady state. Buffers are only freed when the pool is reset in
	 * initialize(), so on an embedded device memory usage is deterministic.
	 */
	struct PooledBuffer {
		CRGB* data;
		uint8_t* opacity;
	};

	std::vector<PooledBuffer> bufferPool;

	PooledBuffer acquireBuffer() {
		if (!bufferPool.empty()) {
			PooledBuffer buffer = bufferPool.back();
			bufferPool.pop_back();
			return buffer;
		}
		return {new CRGB[TemporaryLedData::size], new uint8_t[TemporaryLedData::size]};
	}

	void releaseBuffer(PooledBuffer buffer) { bufferPool.push_back(buffer); }
}

void TemporaryLedData::initialize() {
	// calculate the total number of LEDs.
	TemporaryLedData::startIndexes = new int[FastLED.count()];
	size = 0;
	for (int i = 0; i < FastLED.count(); i++) {
		TemporaryLedData::startIndexes[i] = size;
		size += FastLED[i].size();
	}

	// Drop any pooled buffers: they were sized for a previous `size` and would be too small if re-initialized.
	for (auto& buffer : bufferPool) {
		delete[] buffer.data;
		delete[] buffer.opacity;
	}
	bufferPool.clear();
}

TemporaryLedData::TemporaryLedData(CRGB color) {
	PooledBuffer buffer = acquireBuffer();
	data = buffer.data;
	opacity = buffer.opacity;
	// Buffers are reused, so they carry stale data from a previous owner. Clear to defaults so each effect starts with
	// its own clean buffer and cannot bleed into another effect's pixels.
	for (int i = 0; i < size; i++) {
		(*this)[i] = color;
		opacity[i] = false;
	}
}


TemporaryLedData::TemporaryLedData(TemporaryLedData&& other) noexcept :
	data(other.data), opacity(other.opacity), anyAreModified(other.anyAreModified) {
	// Take ownership of the buffers and null out the source so its destructor releases nothing.
	other.data = nullptr;
	other.opacity = nullptr;
}


TemporaryLedData::~TemporaryLedData() {
	// A moved-from instance owns no buffers; nothing to return to the pool.
	if (data != nullptr) releaseBuffer({data, opacity});
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

	for (int j = 0; j < size; j++) {
		int nearIndex = j - whole;
		int farIndex = j - whole - 1;

		float nearLight = (nearIndex >= 0 && nearIndex < size) ? this->opacity[nearIndex] * (1 - frac) : 0;
		float farLight = (frac > 0 && farIndex >= 0 && farIndex < size) ? this->opacity[farIndex] * frac : 0;

		float totalLight = nearLight + farLight;
		if (totalLight == 0) continue;

		CRGB nearColor = (nearLight > 0) ? this->data[nearIndex] : CRGB::Black;
		CRGB farColor = (farLight > 0) ? this->data[farIndex] : CRGB::Black;

		// Average the two colors weighted by their light share. Weights are normalized to [0, 1] and summed per
		// channel in float so intermediate products never overflow a CRGB channel.
		float nearWeight = nearLight / totalLight;
		float farWeight = farLight / totalLight;
		CRGB blended(nearColor.r * nearWeight + farColor.r * farWeight,
					 nearColor.g * nearWeight + farColor.g * farWeight,
					 nearColor.b * nearWeight + farColor.b * farWeight);

		result.set(j, blended, (uint8_t)min(totalLight, (float)UINT8_MAX));
	}

	return result;
}

void TemporaryLedData::merge(TemporaryLedData& other, BlendingMode blendingMode) {
	for (int i = 0; i < TemporaryLedData::size; i++) {
		auto A_alpha = this->opacity[i];
		auto A_rgb = this->data[i];
		auto B_alpha = other.opacity[i];
		auto B_rgb = other.data[i];
		// if other pixel has no opacity, we skip this pixel. We can't skip it in MASK mode, because in MASK mode
		// opacity of 0 pixels are masked out.
		if (!other.opacity[i] && blendingMode != BlendingMode::MASK) continue;

		this->anyAreModified = true;
		switch (blendingMode) {
		case BlendingMode::OVERWRITE:
			this->data[i] = B_rgb;
			this->opacity[i] = B_alpha;
			break;
		case BlendingMode::ADD:
			this->data[i] = A_rgb + B_rgb;
			this->opacity[i] = min(A_alpha + B_alpha, UINT8_MAX);
			break;
		case BlendingMode::MULTIPLY:
			this->data[i] *= A_rgb * B_rgb;
			this->opacity[i] = (B_alpha * B_alpha) / 255;
			break;
		case BlendingMode::NORMAL:
			this->data[i].r = ((255 - B_alpha) * A_rgb.r + B_alpha * B_rgb.r) / 255;
			this->data[i].g = ((255 - B_alpha) * A_rgb.g + B_alpha * B_rgb.g) / 255;
			this->data[i].b = ((255 - B_alpha) * A_rgb.b + B_alpha * B_rgb.b) / 255;
			this->opacity[i] = B_alpha + ((255 - B_alpha) * A_alpha) / 255;
			break;
		case BlendingMode::MASK:
			// in mask mode, let through everywhere that has 100% opacity
			// and nothing through where the mask has 0% opacity.
			this->opacity[i] = (A_alpha * B_alpha) / 255;
			this->data[i] = (A_rgb * B_rgb);
			break;
		}
	}
}

void TemporaryLedData::set(int index, CRGB color, uint8_t opacity) {
	if (index < 0 || index >= TemporaryLedData::size) return; // LED doesn't exist on specified strip
	this->opacity[index] = opacity;
	anyAreModified = true;
	(*this)[index] = color;
}

void TemporaryLedData::set(int stripIndex, int ledIndex, CRGB& color, uint8_t opacity) {
	if (stripIndex < 0 || stripIndex >= FastLED.count()) return; // strip doesn't exist
	int index = startIndexes[stripIndex] + ledIndex; // index in array
	this->set(index, color, opacity);
}

CRGB TemporaryLedData::get(int index) const {
	if (index < 0 || index >= size) return CRGB::Black;

	return this->data[index];
}

uint8_t TemporaryLedData::getOpacity(int index) const {
	if (index < 0 || index >= size) return 0;

	return this->opacity[index];
}

void TemporaryLedData::populateFastLed() const {
	int currentLed = 0;
	for (int i = 0; i < FastLED.count(); i++) {
		for (int j = 0; j < FastLED[i].size(); (j++, currentLed++)) {
			FastLED[i][j] = CRGB((this->get(currentLed).red * this->getOpacity(currentLed) / 255),
								 (this->get(currentLed).green * this->getOpacity(currentLed) / 255),
								 (this->get(currentLed).blue * this->getOpacity(currentLed) / 255));
		}
	}
}

void TemporaryLedData::printData() const {
	String data = "";
	for (int i = 0; i < size; i++) {
		data += ledpipelines::colorToHex(this->data[i], this->opacity[i]) + " ";
	}
	LPLogger::debug(data);
}
