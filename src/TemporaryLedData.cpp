#include "TemporaryLedData.h"
#include "LedPipelineUtils.h"

using namespace ledpipelines;


int TemporaryLedData::size = 0;

int* TemporaryLedData::startIndexes = nullptr;

void TemporaryLedData::initialize() {
	// calculate the total number of LEDs.
	TemporaryLedData::startIndexes = new int[FastLED.count()];
	size = 0;
	for (int i = 0; i < FastLED.count(); i++) {
		TemporaryLedData::startIndexes[i] = size;
		size += FastLED[i].size();
	}
}

TemporaryLedData::TemporaryLedData(CRGB color) {
	data = new CRGB[size];
	opacity = new uint8_t[size];
	for (int i = 0; i < size; i++) {
		(*this)[i] = color;
		opacity[i] = false;
	}
}


TemporaryLedData::~TemporaryLedData() {
	delete[] data;
	delete[] opacity;
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
