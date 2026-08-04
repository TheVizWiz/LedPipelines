#pragma once

#include <cstdlib>  // rand, RAND_MAX
#include <memory>
#include <utility>
#include <vector>

#include "Color.h"  // RGBA, HSVA
#include "Value.h"
#include "values/RandomSet.h"  // RandomSet<T> - the generic set provider RandomColorSet aliases

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// Randomized color inputs. Both derive from Value<RGBA>, so they drop into any effect input that takes a color
// (Solid, SolidSegment, ...) exactly where a plain RGBA would - no RandomSolid variant needed. A color is sampled once
// per lifecycle and re-rolled when the owning effect is reset().
namespace ledpipelines {
	namespace providers {
		// A random color drawn within a bounding box in RGBA space: each channel (r, g, b, and a) is picked uniformly
		// and independently between min.channel and max.channel. min/max are opposite corners of the box, so the
		// result is a random point inside it. Defaults to Black..White (each channel 0..255) = any color, opaque
		// (alpha 255..255). Per-channel bounds within [min, max] are order-independent (if min.channel > max.channel
		// for some channel, the two are swapped for that channel).
		struct RandomColorProvider : Provider<RGBA> {
			RGBA min;
			RGBA max;

			RandomColorProvider(RGBA min, RGBA max) : min(min), max(max) {}

			RGBA sample() const override {
				return RGBA(channel(min.r, max.r), channel(min.g, max.g), channel(min.b, max.b), channel(min.a, max.a));
			}

		private:
			// One channel drawn uniformly in [lo, hi], inclusive, tolerant of lo > hi.
			static uint8_t channel(uint8_t a, uint8_t b) {
				int lo = a < b ? a : b;
				int hi = a < b ? b : a;
				return (uint8_t)(lo + rand() % (hi - lo + 1));
			}

		public:
#ifdef LEDPIPELINES_SERIALIZATION
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("randomColor") + jsonRawField("min", jsonValue(min)) +
					   jsonRawField("max", jsonValue(max)) + "}";
			}
#endif
		};

		// A random color drawn within a bounding box in HSVA space, then converted to RGBA. Each of h, s, v (floats)
		// and a (byte) is picked uniformly and independently between the min and max corners. Sampling in HSV gives
		// much more natural sweeps than RGB: vary only the hue (e.g. h in 0..360, s=v=1 fixed) for a full spectrum of
		// vivid colors, or lock the hue and vary v for shades of one color - things that are awkward to express as an
		// RGB box. The result is still a plain RGBA, so it drops into any color input like the others.
		struct RandomHSVColorProvider : Provider<RGBA> {
			HSVA min;
			HSVA max;

			RandomHSVColorProvider(HSVA min, HSVA max) : min(min), max(max) {}

			RGBA sample() const override {
				HSVA c(component(min.h, max.h), component(min.s, max.s), component(min.v, max.v),
						channel(min.a, max.a));
				return c.toRGBA();
			}

		private:
			// One float component drawn uniformly in [lo, hi], tolerant of lo > hi.
			static float component(float a, float b) {
				float lo = a < b ? a : b;
				float hi = a < b ? b : a;
				return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
			}

			// One byte channel (alpha) drawn uniformly in [lo, hi], inclusive, tolerant of lo > hi.
			static uint8_t channel(uint8_t a, uint8_t b) {
				int lo = a < b ? a : b;
				int hi = a < b ? b : a;
				return (uint8_t)(lo + rand() % (hi - lo + 1));
			}

		public:
#ifdef LEDPIPELINES_SERIALIZATION
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("randomHsvColor") + jsonRawField("min", jsonValue(min)) +
					   jsonRawField("max", jsonValue(max)) + "}";
			}
#endif
		};

	} // namespace providers

	// Value<RGBA> subclasses so users write the intent directly: Solid::Builder(RandomColor()). They add no data of
	// their own - just construct the right provider - so they pass wherever a Value<RGBA> is expected (the shared_ptr
	// copies intact; nothing to slice).
	struct RandomColor : Value<RGBA> {
		// No bounds -> any opaque color (each of r/g/b in 0..255, alpha 255). With bounds -> each channel drawn between
		// the corresponding channels of min and max, e.g. RandomColor(RGBA(100,0,0), RGBA(255,0,0)) = a random red.
		RandomColor(RGBA min = RGBA(0, 0, 0, 255), RGBA max = RGBA(255, 255, 255, 255))
			: Value<RGBA>(std::make_shared<const providers::RandomColorProvider>(min, max)) {}
	};

	// Random color sampled in HSVA space (see RandomHSVColorProvider). No bounds -> a random vivid hue: hue anywhere on
	// the wheel, full saturation and value, opaque. With bounds -> each of h/s/v/a drawn between min and max, e.g.
	// RandomHSVColor(HSVA(0,1,1), HSVA(60,1,1)) = a random warm color (red..yellow), or lock the hue and vary v for
	// shades of one color.
	struct RandomHSVColor : Value<RGBA> {
		RandomHSVColor(HSVA min = HSVA(0, 1, 1, 255), HSVA max = HSVA(360, 1, 1, 255))
			: Value<RGBA>(std::make_shared<const providers::RandomHSVColorProvider>(min, max)) {}
	};

	// A random RGBA from a fixed palette: the generic RandomSet<RGBA>. Great for a curated look ("red, green, or blue")
	// without the muddiness risk of fully-random RGB. Kept as a named alias so call sites read RandomColorSet({...}).
	using RandomColorSet = RandomSet<RGBA>;
} // namespace ledpipelines
