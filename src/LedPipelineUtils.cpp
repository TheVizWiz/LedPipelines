#include "LedPipelineUtils.h"

using namespace ledpipelines;

CRGB operator*(const CRGB first, const CRGB second) {
	return CRGB((first.r * second.r) / 255, (first.g * second.g) / 255, (first.b * second.b) / 255);
}

CRGB& operator*=(CRGB& first, const CRGB& second) {
	first.r = (first.r * second.r) / 255;
	first.g = (first.g * second.g) / 255;
	first.b = (first.b * second.b) / 255;
	return first;
}

CRGB operator*(const CRGB first, const float amount) {
	return CRGB(first.r * amount, first.g * amount, first.b * amount);
}

CRGB& operator*=(CRGB& first, const float amount) {
	first.r = first.r * amount;
	first.g = first.g * amount;
	first.b = first.b * amount;
	return first;
}

uint64_t ledpipelines::minMicrosBetweenUpdates = 0;

void ledpipelines::setMaxRefreshRate(float refreshesPerSecond) {
	ledpipelines::minMicrosBetweenUpdates = (long)(1000000 / refreshesPerSecond);
}

static char HexLookUp[] = "0123456789abcdef";

static String byteToHex(uint8_t num) {
	uint8_t firstDigit = (num / 0x10);
	uint8_t secondDigit = (num % 0x10);
	return String(HexLookUp[firstDigit]) + HexLookUp[secondDigit];
}

String ledpipelines::colorToHex(CRGB color, uint8_t opacity) {
	return String(byteToHex(color.r) + byteToHex(color.g) + byteToHex(color.b) + byteToHex(opacity));
}
