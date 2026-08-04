#pragma once

#include <cstdlib>  // rand
#include <memory>
#include <utility>
#include <vector>

#include "Value.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// A randomized input that picks one option uniformly from a fixed set on each sample. Fully generic over T: the
// "hold a list, pick one at random" logic is type-agnostic, so a single RandomSet<T> covers colors (RandomSet<RGBA> /
// RandomSet<HSVA>), smoothing curves (RandomSet<SmoothingFunction>), durations, positions - anything with a
// serialization::jsonValue(T) overload. This replaces the former per-type set providers (RandomColorSetProvider,
// RandomHSVASetProvider). Like every Value input, the choice is sampled once per lifecycle and re-rolled on the owning
// effect's reset().
namespace ledpipelines {
	namespace providers {
		template <typename T>
		struct RandomSetProvider : Provider<T> {
			std::vector<T> options;

			explicit RandomSetProvider(std::vector<T> options) : options(std::move(options)) {}

			T sample() const override {
				// Precondition: the set is non-empty (a RandomSet with nothing to choose from is a misuse - there is no
				// value to return, and T need not be default-constructible so we cannot synthesize one). rand() % size
				// picks a uniform index.
				return options[rand() % options.size()];
			}

#ifdef LEDPIPELINES_SERIALIZATION
			// Serializes as {"provider":"randomSet","options":[<bare value>, ...]}. Requires a serialization::jsonValue
			// overload for T (the bare-value formatter) - the same one ConstantProvider<T> needs.
			std::string describeValue() const override {
				using namespace serialization;
				std::string arr = "[";
				for (size_t i = 0; i < options.size(); i++) {
					if (i > 0) arr += ",";
					arr += jsonValue(options[i]);
				}
				arr += "]";
				return "{" + jsonProvider("randomSet") + jsonRawField("options", arr) + "}";
			}
#endif
		};
	} // namespace providers

	// Value<T> wrapper so users write the intent directly: Solid::Builder(RandomSet<RGBA>({RGBA::Red, RGBA::Blue})),
	// FadeOut::Builder(1000).smoothingFunction(RandomSet<SmoothingFunction>({SmoothingFunction::SINE,
	// SmoothingFunction::QUADRATIC})). Adds no data of its own - just constructs the provider - so it passes wherever a
	// Value<T> is expected.
	template <typename T>
	struct RandomSet : Value<T> {
		explicit RandomSet(std::vector<T> options)
			: Value<T>(std::make_shared<const providers::RandomSetProvider<T>>(std::move(options))) {}
	};
} // namespace ledpipelines
