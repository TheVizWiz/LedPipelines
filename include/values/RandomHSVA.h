#pragma once

#include <cstdlib>  // rand, RAND_MAX
#include <cstdint>  // uint8_t
#include <memory>
#include <utility>
#include <vector>

#include "Color.h"  // HSVA
#include "Value.h"
#include "values/RandomSet.h"  // RandomSet<T> - the generic set provider RandomHSVASet aliases

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// Randomized color inputs that return HSVA (not RGBA). These are the Value<HSVA> siblings of the Value<RGBA> family in
// RandomColor.h, for effects whose color inputs are natively HSVA - chiefly HSVGradient, whose corners interpolate in
// HSV space. A plain HSVA converts implicitly to Value<HSVA> (a constant, via the generic ConstantProvider), so no
// named "constant" type is needed here; this file adds the randomized recipes.
//
// Naming: the "HSVA" token marks types that RETURN HSVA, distinguishing them from RandomHSVColor in RandomColor.h,
// which SAMPLES in HSV space but returns RGBA. A color is sampled once per lifecycle and re-rolled on the owning
// effect's reset(), like every other Value input.
namespace ledpipelines {
	namespace providers {
		// A random color drawn within a bounding box in HSVA space, returned as raw HSVA. Each of h, s, v (floats) and
		// a (byte) is picked uniformly and independently between the min and max corners. This is the HSVA analog of
		// RandomHSVColorProvider in RandomColor.h - the box-draw is identical, but that one converts to RGBA (it's a
		// Value<RGBA>) while this returns HSVA so it drops straight into an HSVGradient corner without a round-trip
		// through RGB. Sampling in HSV gives natural sweeps: vary only the hue (h in 0..360, s=v=1) for a full spectrum,
		// or lock the hue and vary v for shades of one color. (Draw logic is duplicated from RandomHSVColorProvider by
		// choice, keeping the two providers decoupled; could be shared later.)
		struct RandomHSVAProvider : Provider<HSVA> {
			HSVA min;
			HSVA max;

			RandomHSVAProvider(HSVA min, HSVA max) : min(min), max(max) {}

			HSVA sample() const override {
				return HSVA(component(min.h, max.h), component(min.s, max.s), component(min.v, max.v),
							 channel(min.a, max.a));
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
				return "{" + jsonProvider("randomHsva") + jsonRawField("min", jsonValue(min)) +
					   jsonRawField("max", jsonValue(max)) + "}";
			}
#endif
		};

	} // namespace providers

	// Random HSVA drawn within HSVA bounds (see RandomHSVAProvider). The HSVA-returning sibling of RandomHSVColor in
	// RandomColor.h. No bounds -> a random vivid hue: hue anywhere on the wheel, full saturation and value, opaque. With
	// bounds -> each of h/s/v/a drawn between min and max, e.g. RandomHSVA(HSVA(0,1,1), HSVA(60,1,1)) = a random warm
	// color (red..yellow), or lock the hue and vary v for shades of one color.
	struct RandomHSVA : Value<HSVA> {
		RandomHSVA(HSVA min = HSVA(0, 1, 1, 255), HSVA max = HSVA(360, 1, 1, 255))
			: Value<HSVA>(std::make_shared<const providers::RandomHSVAProvider>(min, max)) {}
	};

	// A random HSVA from a fixed palette: the generic RandomSet<HSVA>. A curated set of HSV colors ("this red, this
	// orange, this yellow") for HSVGradient corners. Kept as a named alias so call sites read RandomHSVASet({...}).
	using RandomHSVASet = RandomSet<HSVA>;
} // namespace ledpipelines
