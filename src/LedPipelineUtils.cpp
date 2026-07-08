#include "LedPipelineUtils.h"

#include <cmath>

using namespace ledpipelines;

CRGB fhsvToRgb(FHSV hsv) {
	// Wrap hue into [0, 360) and clamp saturation/value into [0, 1].
	float h = hsv.h - 360.0f * floorf(hsv.h / 360.0f);
	float s = hsv.s < 0 ? 0 : (hsv.s > 1 ? 1 : hsv.s);
	float v = hsv.v < 0 ? 0 : (hsv.v > 1 ? 1 : hsv.v);

	// Standard HSV -> RGB, hue split into six 60-degree sectors.
	float c = v * s;
	float hPrime = h / 60.0f;
	float x = c * (1 - fabsf(fmodf(hPrime, 2.0f) - 1));
	float m = v - c;

	float r1, g1, b1;
	switch ((int)hPrime) {
		case 0:  r1 = c; g1 = x; b1 = 0; break;
		case 1:  r1 = x; g1 = c; b1 = 0; break;
		case 2:  r1 = 0; g1 = c; b1 = x; break;
		case 3:  r1 = 0; g1 = x; b1 = c; break;
		case 4:  r1 = x; g1 = 0; b1 = c; break;
		default: r1 = c; g1 = 0; b1 = x; break; // case 5 and hPrime == 6 edge (h just under 360)
	}

	return CRGB(
		(uint8_t)((r1 + m) * 255 + 0.5f),
		(uint8_t)((g1 + m) * 255 + 0.5f),
		(uint8_t)((b1 + m) * 255 + 0.5f)
	);
}

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
