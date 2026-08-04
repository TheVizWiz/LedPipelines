#pragma once

#include <memory>
#include <utility>

#include "Value.h"
#include "enums/SamplingFunction.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// A randomized numeric input drawn from a [min, max] range, generic over the numeric type T. RandomRange<T> is a
// Value<T> (mirroring RandomSet<T>), so it drops into any numeric effect input exactly where a plain number would - a
// duration/count (RandomRange<unsigned long> for runtimeMs/spawnTimeMs), a position/offset (RandomRange<float> for a
// Shift offset or a gradient endpoint), a byte (RandomRange<uint8_t> for OpacityScale's maxOpacity), an index
// (RandomRange<long> for Flip), and so on. The number is sampled once per lifecycle and re-rolled when the owning effect
// is reset() (see e.g. TimedEffect::resetTimer). The spread between min and max is shaped by a SamplingFunction (UNIFORM
// by default; CENTERED/EDGES/custom to bias the draw).
//
// The type must MATCH the target field's Value<T>: a field declared Value<long> needs RandomRange<long>, not
// RandomRange<int> (a Value<int> does not convert to a Value<long>). CTAD from RandomRange(20, 40) deduces
// RandomRange<int>, so pass the type explicitly (RandomRange<long>(20, 40)) to match a specific field. This mirrors
// RandomSet<T>, which is likewise an explicitly-typed Value<T>.
namespace ledpipelines {
	namespace providers {
		template <typename T>
		struct RandomRangeProvider : Provider<T> {
			T min;
			T max;
			SamplingFunction samplingFunction;

			RandomRangeProvider(T min, T max, SamplingFunction samplingFunction)
				: min(min), max(max), samplingFunction(std::move(samplingFunction)) {}

			T sample() const override {
				// SamplingFunction works in float; the (T) cast truncates toward zero for an integral T (so a duration/
				// count lands on a whole unit) and is the identity for a floating-point T.
				return (T)samplingFunction((float)min, (float)max);
			}

#ifdef LEDPIPELINES_SERIALIZATION
			// Serializes as {"provider":"randomRange","min":..,"max":..,"samplingFunction":".."}. min/max render via the
			// jsonValue(T) overload for T (so an integral range emits 5, a float range 5.0) - the same overload
			// ConstantProvider<T> needs.
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("randomRange") + jsonRawField("min", jsonValue(min)) +
					   jsonRawField("max", jsonValue(max)) + jsonField("samplingFunction", samplingFunction) + "}";
			}
#endif
		};
	} // namespace providers

	// Value<T> wrapper so users write the intent directly: FadeOut::Builder(RandomRange<unsigned long>(500, 2000)),
	// Shift::Builder(RandomRange<float>(-10, 10)). Adds no data of its own - just constructs the provider - so it passes
	// wherever a Value<T> is expected (the shared_ptr copies intact; nothing to slice).
	template <typename T>
	struct RandomRange : Value<T> {
		RandomRange(T min, T max, SamplingFunction samplingFunction = SamplingFunction::UNIFORM)
			: Value<T>(std::make_shared<const providers::RandomRangeProvider<T>>(min, max, std::move(samplingFunction))) {}
	};
} // namespace ledpipelines
