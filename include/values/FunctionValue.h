#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "Value.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// Values computed by a user-supplied function, which must be non-empty and safe to call at build time.
//   - FunctionValue calls it each time it is sampled: once when constructed and again on every reset() of the owning
//     effect. Reads in between return that cached result.
//   - LiveFunctionValue additionally calls it on every read (Value::get()), so it can track changing state such as a
//     sensor; each call also refreshes the cached value.
//
// T must MATCH the target field's Value<T>; CTAD deduces it from the function's return type, so a lambda returning
// int will not fit a Value<long> field - spell the type out or return the right type.
namespace ledpipelines {
	namespace providers {
		template <typename T>
		struct FunctionProvider : Provider<T> {
			std::function<T()> function;

			explicit FunctionProvider(std::function<T()> function) : function(std::move(function)) {}

			T sample() const override {
				return function();
			}

#ifdef LEDPIPELINES_SERIALIZATION
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("function") + "}";
			}
#endif
		};

		template <typename T>
		struct LiveFunctionProvider : FunctionProvider<T> {
			using FunctionProvider<T>::FunctionProvider;

			const T& get(T& cached) const override {
				cached = this->function();
				return cached;
			}

#ifdef LEDPIPELINES_SERIALIZATION
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("liveFunction") + "}";
			}
#endif
		};
	}

	template <typename T>
	struct FunctionValue : Value<T> {
		explicit FunctionValue(std::function<T()> function)
			: Value<T>(std::make_shared<const providers::FunctionProvider<T>>(std::move(function))) {}
	};

	template <typename F>
	FunctionValue(F) -> FunctionValue<std::decay_t<std::invoke_result_t<F&>>>;

	template <typename T>
	struct LiveFunctionValue : Value<T> {
		explicit LiveFunctionValue(std::function<T()> function)
			: Value<T>(std::make_shared<const providers::LiveFunctionProvider<T>>(std::move(function))) {}
	};

	template <typename F>
	LiveFunctionValue(F) -> LiveFunctionValue<std::decay_t<std::invoke_result_t<F&>>>;
}
