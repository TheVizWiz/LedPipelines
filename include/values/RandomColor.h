#pragma once

#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

#include "Color.h"
#include "Value.h"
#include "values/RandomSet.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// Randomized Value<RGBA> inputs. A color is sampled once per lifecycle and re-rolled when the owning effect is reset().
namespace ledpipelines {
	namespace providers {
		// Each channel (including alpha) is drawn uniformly and independently between min and max; per-channel bounds
		// are order-independent.
		struct RandomColorProvider : Provider<RGBA> {
			RGBA min;
			RGBA max;

			RandomColorProvider(RGBA min, RGBA max) : min(min), max(max) {}

			RGBA sample() const override {
				return RGBA(channel(min.r, max.r), channel(min.g, max.g), channel(min.b, max.b), channel(min.a, max.a));
			}

		private:
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

		// Each of h, s, v, a is drawn uniformly and independently between min and max in HSVA space, then converted
		// to RGBA.
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
			static float component(float a, float b) {
				float lo = a < b ? a : b;
				float hi = a < b ? b : a;
				return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
			}

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

	}

	struct RandomColor : Value<RGBA> {
		// Defaults to any opaque color. See RandomColorProvider.
		RandomColor(RGBA min = RGBA(0, 0, 0, 255), RGBA max = RGBA(255, 255, 255, 255))
			: Value<RGBA>(std::make_shared<const providers::RandomColorProvider>(min, max)) {}
	};

	// Samples in HSVA space but returns RGBA (see RandomHSVColorProvider). Defaults to a random fully-saturated,
	// full-value, opaque hue.
	struct RandomHSVColor : Value<RGBA> {
		RandomHSVColor(HSVA min = HSVA(0, 1, 1, 255), HSVA max = HSVA(360, 1, 1, 255))
			: Value<RGBA>(std::make_shared<const providers::RandomHSVColorProvider>(min, max)) {}
	};

	using RandomColorSet = RandomSet<RGBA>;
}
