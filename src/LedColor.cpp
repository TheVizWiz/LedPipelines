#include "LedColor.h"

#include <cmath>

RGBA HSVA::toRGBA() {
	const float h = this->h - 360.0f * floorf(this->h / 360.0f);
	const float s = this->s < 0 ? 0 : (this->s > 1 ? 1 : this->s);
	const float v = this->v < 0 ? 0 : (this->v > 1 ? 1 : this->v);

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
		break;
	}

	return RGBA(static_cast<uint8_t>((r1 + m) * 255 + 0.5f),
				static_cast<uint8_t>((g1 + m) * 255 + 0.5f),
				static_cast<uint8_t>((b1 + m) * 255 + 0.5f),
				this->a);
}

HSVA::HSVA(const RGBA& rgba) {
	const float r = rgba.r / 255.0f;
	const float g = rgba.g / 255.0f;
	const float b = rgba.b / 255.0f;

	const float maxC = r > g ? (r > b ? r : b) : (g > b ? g : b);
	const float minC = r < g ? (r < b ? r : b) : (g < b ? g : b);
	const float delta = maxC - minC;

	float hue;
	if (delta == 0) {
		hue = 0;
	} else if (maxC == r) {
		hue = 60.0f * fmodf((g - b) / delta, 6.0f);
	} else if (maxC == g) {
		hue = 60.0f * (((b - r) / delta) + 2.0f);
	} else {
		hue = 60.0f * (((r - g) / delta) + 4.0f);
	}
	if (hue < 0) hue += 360.0f;

	this->h = hue;
	this->s = maxC == 0 ? 0 : delta / maxC;
	this->v = maxC;
	this->a = rgba.a;
}

HSVA::HSVA(uint32_t rgbCode) : HSVA(RGBA(rgbCode)) {}

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
