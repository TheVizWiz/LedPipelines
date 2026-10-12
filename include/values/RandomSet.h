#pragma once

#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

#include "Value.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// A Value<T> that picks one option uniformly from a fixed, non-empty set. Sampled once per lifecycle and re-rolled on
// the owning effect's reset(). Serialization requires a serialization::jsonValue(T) overload.
namespace ledpipelines {
	namespace providers {
		template <typename T>
		struct RandomSetProvider : Provider<T> {
			std::vector<T> options;

			explicit RandomSetProvider(std::vector<T> options) : options(std::move(options)) {}

			T sample() const override {
				// Precondition: non-empty. T need not be default-constructible, so there is no fallback value.
				return options[rand() % options.size()];
			}

#ifdef LEDPIPELINES_SERIALIZATION
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
	}

	template <typename T>
	struct RandomSet : Value<T> {
		explicit RandomSet(std::vector<T> options)
			: Value<T>(std::make_shared<const providers::RandomSetProvider<T>>(std::move(options))) {}
	};
}
