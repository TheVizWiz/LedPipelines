#pragma once

// Minimal host-side (native) stub of FastLED, sufficient to compile and link LedPipelines in env:local for unit tests.
// Provides only the surface the library uses: the CRGB color struct (with the r/g/b and red/green/blue member views,
// named colors, and the arithmetic operators the library defines elsewhere), and a fake FastLED controller exposing
// count()/operator[]/clear()/show() plus per-strip size()/operator[]. Real rendering is only exercised on-device.

#include <cstdint>
#include <vector>

#include "Arduino.h"

struct CRGB {
	// FastLED exposes the channels under two names (r/g/b and red/green/blue) via a union; the library uses both.
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};
		struct {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
		};
		uint8_t raw[3];
	};

	CRGB() : r(0), g(0), b(0) {}
	CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

	// Named colors used by the library/tests.
	enum HTMLColorCode : uint32_t {
		Black = 0x000000,
		Red = 0xFF0000,
		Green = 0x008000,
		Blue = 0x0000FF,
		White = 0xFFFFFF,
	};

	CRGB(uint32_t code) : r((code >> 16) & 0xFF), g((code >> 8) & 0xFF), b(code & 0xFF) {}

	CRGB operator+(const CRGB &o) const {
		auto clamp = [](int v) { return (uint8_t)(v > 255 ? 255 : v); };
		return CRGB(clamp(r + o.r), clamp(g + o.g), clamp(b + o.b));
	}
};

// A single logical strip: a window into a caller-provided CRGB buffer.
struct FakeStrip {
	CRGB *leds;
	int len;

	int size() const { return len; }
	CRGB &operator[](int i) { return leds[i]; }
};

// The global FastLED controller stand-in: holds the strips registered via addLeds<>().
struct FakeController {
	std::vector<FakeStrip> strips;

	// Template signature mirrors FastLED.addLeds<CHIPSET, PIN, ORDER>(leds, count) and the offset overload; the
	// template params are ignored on the host.
	template <typename... Args>
	FakeStrip &addLeds(CRGB *leds, int count) {
		strips.push_back(FakeStrip{leds, count});
		return strips.back();
	}

	template <typename... Args>
	FakeStrip &addLeds(CRGB *leds, int offset, int count) {
		strips.push_back(FakeStrip{leds + offset, count});
		return strips.back();
	}

	int count() const { return (int)strips.size(); }
	FakeStrip &operator[](int i) { return strips[i]; }

	void clear() {}
	void show() {}

	// Test-only helper to reset registered strips between tests.
	void reset() { strips.clear(); }
};

extern FakeController FastLED;
