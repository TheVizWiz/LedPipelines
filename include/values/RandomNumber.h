#pragma once

#include <memory>
#include <utility>

#include "Value.h"
#include "enums/SamplingFunction.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// A randomized numeric Value<T> drawn from [min, max], shaped by a SamplingFunction. Sampled once per lifecycle and
// re-rolled when the owning effect is reset(). Integral T truncates toward zero.
//
// T must MATCH the target field's Value<T>: a Value<long> field needs RandomRange<long>, not RandomRange<int> (a
// Value<int> does not convert to a Value<long>), and CTAD from RandomRange(20, 40) deduces int - so spell the type out.
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
				return (T)samplingFunction((float)min, (float)max);
			}

#ifdef LEDPIPELINES_SERIALIZATION
			// Requires a serialization::jsonValue(T) overload.
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("randomRange") + jsonRawField("min", jsonValue(min)) +
					   jsonRawField("max", jsonValue(max)) + jsonField("samplingFunction", samplingFunction) + "}";
			}
#endif
		};
	}

	template <typename T>
	struct RandomRange : Value<T> {
		RandomRange(T min, T max, SamplingFunction samplingFunction = SamplingFunction::UNIFORM)
			: Value<T>(std::make_shared<const providers::RandomRangeProvider<T>>(min, max, std::move(samplingFunction))) {}
	};
}
