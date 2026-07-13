#include "Color.h"

#include <cmath>

RGBA FHSV::toRGBA() {

	// Wrap hue into [0, 360) and clamp saturation/value into [0, 1].
	const float h = this->h - 360.0f * floorf(this->h / 360.0f);
	const float s = this->s < 0 ? 0 : (this->s > 1 ? 1 : this->s);
	const float v = this->v < 0 ? 0 : (this->v > 1 ? 1 : this->v);

	// Standard HSV -> RGB, hue split into six 60-degree sectors.
	float c = v * s;
	float hPrime = h / 60.0f;
	float x = c * (1 - fabsf(fmodf(hPrime, 2.0f) - 1));
	float m = v - c;

	float r1, g1, b1;
	switch (static_cast<int>(hPrime)) {
	case 0:
		r1 = c;
		g1 = x;
		b1 = 0;
		break;
	case 1:
		r1 = x;
		g1 = c;
		b1 = 0;
		break;
	case 2:
		r1 = 0;
		g1 = c;
		b1 = x;
		break;
	case 3:
		r1 = 0;
		g1 = x;
		b1 = c;
		break;
	case 4:
		r1 = x;
		g1 = 0;
		b1 = c;
		break;
	default:
		r1 = c;
		g1 = 0;
		b1 = x;
		break; // case 5 and hPrime == 6 edge (h just under 360)
	}

	return RGBA(static_cast<uint8_t>((r1 + m) * 255 + 0.5f),
				static_cast<uint8_t>((g1 + m) * 255 + 0.5f),
				static_cast<uint8_t>((b1 + m) * 255 + 0.5f));
}

RGBA& RGBA::operator=(uint32_t code) {
	r = (code >> 16) & 0xFF;
	g = (code >> 8) & 0xFF;
	b = code & 0xFF;
	return *this;
}
RGBA RGBA::operator*(RGBA second) const {
	return RGBA((this->r * second.r) / 255, (this->g * second.g) / 255, (this->b * second.b) / 255);
}

RGBA RGBA::operator*(float amount) const {
	return RGBA(this->r * amount, this->g * amount, this->b * amount);
}


RGBA& RGBA::operator*=(float amount) {
	this->r = this->r * amount;
	this->g = this->g * amount;
	this->b = this->b * amount;
	return *this;
}

RGBA RGBA::operator+(RGBA second) {
	// Saturating add: clamp each channel at 255 so a bright + bright doesn't wrap around.
	int newR = this->r + second.r;
	int newG = this->g + second.g;
	int newB = this->b + second.b;
	return RGBA(
		(uint8_t)(newR > 255 ? 255 : newR), (uint8_t)(newG > 255 ? 255 : newG), (uint8_t)(newB > 255 ? 255 : newB));
}

RGBA& RGBA::operator*=(RGBA second) {
	this->r = (this->r * second.r) / 255;
	this->g = (this->g * second.g) / 255;
	this->b = (this->b * second.b) / 255;
	return *this;
}
