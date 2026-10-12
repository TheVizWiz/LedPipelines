#pragma once

#include <memory>
#include <type_traits>
#include <utility>

/**
 * Lets effect parameters be a "recipe" rather than a single hardcoded value (e.g. a background color randomly chosen
 * from red, green, and blue).
 *
 * Provider<T> samples values; it is typically sampled once per effect lifecycle. Concrete providers include
 * ConstantProvider (always the same value) and random providers (a range, or a set of values).
 *
 * Value<T> is a copyable wrapper around a shared Provider<T> plus its own cached sample.
 */

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

namespace ledpipelines {
	template <typename T>
	struct Provider {
		virtual ~Provider() = default;

		virtual T sample() const = 0;

#ifdef LEDPIPELINES_SERIALIZATION
		// Raw JSON value fragment (no leading key) describing this recipe.
		virtual std::string describeValue() const = 0;
#endif
	};

	template <typename T>
	struct ConstantProvider : Provider<T> {
		T value;

		explicit ConstantProvider(T value) : value(std::move(value)) {}

		T sample() const override {
			return value;
		}

#ifdef LEDPIPELINES_SERIALIZATION
		// Renders {"provider":"constant","value":<bare value>}. Requires a serialization::jsonValue overload for T.
		std::string describeValue() const override {
			using namespace serialization;
			return "{" + jsonProvider("constant") + jsonRawField("value", jsonValue(value)) + "}";
		}
#endif
	};

	template <typename T>
	class Value {
		// The provider is shared between copies, but each copy caches its own draw in `value`, so one recipe (e.g. a
		// single RandomColor) reused across effects still gives each effect an independent value.
		std::shared_ptr<const Provider<T>> provider;

	public:
		// The last value drawn by sample(); the owning effect reads this every frame. Primed by every constructor.
		T value;

		/**
		 * The converting constructor accepts anything convertible to T, allowing two implicit hops such as
		 * HTMLColorCode -> RGBA -> Value<RGBA>. The default constructor holds T{} and is needed by BUILDER_FIELD.
		 *
		 * provider is declared before value, so value(provider->sample()) is well-ordered.
		 */
		Value() : provider(std::make_shared<const ConstantProvider<T>>(T{})), value(provider->sample()) {}

		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
		Value(U&& constant)
			: provider(std::make_shared<const ConstantProvider<T>>(T(std::forward<U>(constant)))),
			  value(provider->sample()) {}

		Value(std::shared_ptr<const Provider<T>> p) : provider(std::move(p)), value(provider->sample()) {}

		// Draw a fresh value (a random provider re-rolls) and cache it in `value`. The owning effect calls this at
		// lifecycle boundaries (constructor and reset()).
		T sample() {
			value = provider->sample();
			return value;
		}

#ifdef LEDPIPELINES_SERIALIZATION
		std::string describeValue() const {
			return provider->describeValue();
		}
#endif
	};
}
