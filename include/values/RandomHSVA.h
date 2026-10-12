#pragma once

#include <cstdlib>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "LedColor.h"
#include "Value.h"
#include "values/RandomSet.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// Randomized Value<HSVA> inputs (e.g. for HSVGradient corners). Unlike RandomHSVColor in RandomColor.h, which samples
// in HSV but returns RGBA, these RETURN HSVA. Sampled once per lifecycle and re-rolled on the owning effect's reset().
namespace ledpipelines {
	namespace providers {
		// Each of h, s, v, a is drawn uniformly and independently between min and max; bounds are order-independent.
		struct RandomHSVAProvider : Provider<HSVA> {
			HSVA min;
			HSVA max;

			RandomHSVAProvider(HSVA min, HSVA max) : min(min), max(max) {}

			HSVA sample() const override {
				return HSVA(component(min.h, max.h), component(min.s, max.s), component(min.v, max.v),
							 channel(min.a, max.a));
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
				return "{" + jsonProvider("randomHsva") + jsonRawField("min", jsonValue(min)) +
					   jsonRawField("max", jsonValue(max)) + "}";
			}
#endif
		};

	}

	// Defaults to a random fully-saturated, full-value, opaque hue. See RandomHSVAProvider.
	struct RandomHSVA : Value<HSVA> {
		RandomHSVA(HSVA min = HSVA(0, 1, 1, 255), HSVA max = HSVA(360, 1, 1, 255))
			: Value<HSVA>(std::make_shared<const providers::RandomHSVAProvider>(min, max)) {}
	};

	using RandomHSVASet = RandomSet<HSVA>;
}
