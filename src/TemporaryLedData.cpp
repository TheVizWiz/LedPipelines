#include "TemporaryLedData.h"
#include "LedOutput.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ledpipelines;

int TemporaryLedData::size = 0;

int TemporaryLedData::padding = 0;

int TemporaryLedData::bufferSize = 0;

int* TemporaryLedData::startIndexes = nullptr;

namespace {
	/**
	 * Free list of backing arrays, to avoid per-frame new[]/delete[] (heap fragmentation and jitter on
	 * microcontrollers). Grows to the high-water mark and then stops allocating; buffers are only freed in
	 * initialize().
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
}

void TemporaryLedData::initialize() {
	LedOutput* output = getOutput();
	if (output == nullptr) {
		return;
	}

	TemporaryLedData::startIndexes = new int[output->stripCount()];
	size = 0;
	for (int i = 0; i < output->stripCount(); i++) {
		TemporaryLedData::startIndexes[i] = size;
		size += output->stripSize(i);
	}

	padding = size;
	bufferSize = size + 2 * padding;

	// Pooled buffers were sized for the previous `size`.
	for (auto& buffer : bufferPool) {
		delete[] buffer;
	}
	bufferPool.clear();
}

TemporaryLedData::TemporaryLedData(RGBA color) {
	data = acquireBuffer();
	// Pooled buffers carry stale data from a previous owner.
	clear(color);
}

void TemporaryLedData::clear(RGBA color) {
	color.a = 0;
	for (int i = 0; i < bufferSize; i++) {
		data[i] = color;
	}
	anyAreModified = false;
}

TemporaryLedData::TemporaryLedData(TemporaryLedData&& other) noexcept
	: data(other.data), anyAreModified(other.anyAreModified) {
	other.data = nullptr;
}

TemporaryLedData::~TemporaryLedData() {
	if (data != nullptr) releaseBuffer(data);
}

TemporaryLedData TemporaryLedData::shift(float offset) const {
	TemporaryLedData result = TemporaryLedData();

	// Output j draws from sources (j - whole) with weight (1 - frac) and (j - whole - 1) with weight frac. The color
	// is averaged by each source's light (opacity * weight) so partial coverage dims via opacity rather than tinting
	// toward black; the output opacity is the summed light.
	int whole = (int)floorf(offset);
	float frac = offset - whole;

	for (int j = 0; j < bufferSize; j++) {
		int nearIndex = j - whole;
		int farIndex = j - whole - 1;

		float nearLight = (nearIndex >= 0 && nearIndex < bufferSize) ? this->data[nearIndex].a * (1 - frac) : 0;
		float farLight = (frac > 0 && farIndex >= 0 && farIndex < bufferSize) ? this->data[farIndex].a * frac : 0;

		float totalLight = nearLight + farLight;
		if (totalLight == 0) continue;

		RGBA nearColor = (nearLight > 0) ? this->data[nearIndex] : RGBA(RGBA::Black);
		RGBA farColor = (farLight > 0) ? this->data[farIndex] : RGBA(RGBA::Black);

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
	for (int i = 0; i < TemporaryLedData::bufferSize; i++) {
		auto A_alpha = this->data[i].a;
		auto A_rgb = this->data[i];
		auto B_alpha = other.data[i].a;
		auto B_rgb = other.data[i];
		// Can't skip in MASK mode: zero-opacity mask pixels must mask out the destination.
		if (!B_alpha && blendingMode != BlendingMode::MASK) continue;

		this->anyAreModified = true;
		switch (blendingMode) {
		case BlendingMode::OVERWRITE:
			this->data[i] = B_rgb;
			this->data[i].a = B_alpha;
			break;
		case BlendingMode::ADD: {
			// Sum the visible (alpha-weighted) light, then un-premultiply by the combined alpha.
			const int outAlpha = std::min(A_alpha + B_alpha, (int)UINT8_MAX);
			auto add = [&](uint8_t a, uint8_t b) {
				const int light = std::min((a * A_alpha + b * B_alpha + 127) / 255, (int)UINT8_MAX);
				return (uint8_t)std::min((light * 255 + outAlpha / 2) / outAlpha, (int)UINT8_MAX);
			};
			this->data[i].r = add(A_rgb.r, B_rgb.r);
			this->data[i].g = add(A_rgb.g, B_rgb.g);
			this->data[i].b = add(A_rgb.b, B_rgb.b);
			this->data[i].a = outAlpha;
			break;
		}
		case BlendingMode::MULTIPLY: {
			// Tint the destination by the source color, by the source's opacity; coverage stays the destination's.
			auto multiply = [&](uint8_t a, uint8_t b) {
				return (uint8_t)((a * (255 - B_alpha) + (a * b / 255) * B_alpha + 127) / 255);
			};
			this->data[i].r = multiply(A_rgb.r, B_rgb.r);
			this->data[i].g = multiply(A_rgb.g, B_rgb.g);
			this->data[i].b = multiply(A_rgb.b, B_rgb.b);
			break;
		}
		case BlendingMode::NORMAL: {
			// Colors are stored un-premultiplied (populate() applies alpha), so weight each layer by its contribution
			// and divide by the result's alpha; otherwise alpha is applied twice over a transparent destination.
			const int outAlpha = B_alpha + ((255 - B_alpha) * A_alpha) / 255;
			const int bWeight = B_alpha * 255;
			const int aWeight = A_alpha * (255 - B_alpha);
			const int total = outAlpha * 255;
			this->data[i].r = (B_rgb.r * bWeight + A_rgb.r * aWeight + total / 2) / total;
			this->data[i].g = (B_rgb.g * bWeight + A_rgb.g * aWeight + total / 2) / total;
			this->data[i].b = (B_rgb.b * bWeight + A_rgb.b * aWeight + total / 2) / total;
			this->data[i].a = outAlpha;
			break;
		}
		case BlendingMode::MASK: {
			// The mask's visible brightness gates the destination's opacity; the destination keeps its own color.
			const int gate = std::max({B_rgb.r, B_rgb.g, B_rgb.b}) * B_alpha / 255;
			this->data[i].a = (A_alpha * gate + 127) / 255;
			break;
		}
		}
	}
}

bool TemporaryLedData::hasVisiblePixels() const {
	for (int i = padding; i < padding + size; i++) {
		if (data[i].a) return true;
	}
	return false;
}

void TemporaryLedData::set(int index, RGBA color, uint8_t opacity) {
	if (index < -padding || index >= size + padding) return;
	int physical = index + padding;
	color.a = opacity;
	this->data[physical] = color;
	anyAreModified = true;
}

void TemporaryLedData::set(int stripIndex, int ledIndex, RGBA& color, uint8_t opacity) {
	LedOutput* output = getOutput();
	if (output == nullptr || stripIndex < 0 || stripIndex >= output->stripCount()) return;
	int index = startIndexes[stripIndex] + ledIndex;
	this->set(index, color, opacity);
}

RGBA TemporaryLedData::get(int index) const {
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
