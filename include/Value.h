#pragma once

#include <memory>
#include <type_traits>
#include <utility>


/**
 * Many times, for effects, we want to introduce a set of possible values for different parameters instead of a single
 * hardcoded value. For example, let's say you want to have a strip with a background color that randomly selects
 * between red, green, and blue. Usually, to do this, you'd have to write custom code. To support these use cases
 * in LedPipelines, we introduce two new types: a Value and a Provider.
 *
 * Provider<T> is an abstract base class that provides a way to sample values. These values get sampled once per
 * lifetime. A Provider<float> will sample a float, which can be used to get, for example, a position along the strip.
 * A Provider<float> can be inherited by multiple concrete providers, such as a ConstantProvider<float> (returns the
 * same value every time); a RandomProvider<float> (provides a random value between two bounds);
 * or a RandomSetProvider<float> (provides a float from a given list of values).
 *
 * Value<T> is a wrapper around a Provider<T> that provides a standardized way to get a provided value without
 * having to worry about which provider is being used or worry about pointers or anything like that. You can copy
 * a Value<T>.
 *
 */

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h" // serialization::jsonValue(T) - how a constant renders itself
#endif

namespace ledpipelines {
	template <typename T>
	struct Provider {
		virtual ~Provider() = default;

		/**
		 * Get a value from the provider. Usually only called once per lifecycle.
		 */
		virtual T sample() const = 0;

#ifdef LEDPIPELINES_SERIALIZATION
		// Describe this recipe as a raw JSON value fragment (no leading key), for the effect's config serialization.
		// A constant renders as its plain value (e.g. an RGBA object); random providers render a {"random":...} spec.
		virtual std::string describeValue() const = 0;
#endif
	};

	// The trivial recipe: always the same value.
	template <typename T>
	struct ConstantProvider : Provider<T> {
		T value;

		explicit ConstantProvider(T value) : value(std::move(value)) {}

		T sample() const override {
			return value;
		}

#ifdef LEDPIPELINES_SERIALIZATION
		// A constant is a tagged provider like any other: {"provider":"constant","value":<bare value>}. Requires a
		// serialization::jsonValue overload for T (the bare-value formatter).
		std::string describeValue() const override {
			using namespace serialization;
			return "{" + jsonProvider("constant") + jsonRawField("value", jsonValue(value)) + "}";
		}
#endif
	};

	template <typename T>
	class Value {
		// The recipe is shared (immutable, refcounted): copying a Value copies this pointer, so many Values can share
		// one provider. The cache is NOT shared - it is a plain by-value member, so each copy caches its own draw. That
		// is what lets one recipe (e.g. a single RandomColor) be reused across several effects while each effect samples
		// its own independent value.
		std::shared_ptr<const Provider<T>> provider;

	public:
		// The last value drawn from the provider (by sample()). The owning effect reads this every frame instead of
		// keeping its own sampled copy - the Value IS the cache. Primed by every constructor so it is valid immediately.
		T value;

		/**
		 * Two default constructors: one with no arguments, and one that takes anything that can convert to the value
		 * type T. This covers any case where we would provide a T to make a Value<T>. For example, this lets us
		 * use Value<RGBA> red = RGBA::Red, even though that is technically two implicit jumps
		 * (HTMLColorCode -> RGBA -> Value<RGBA>). The no argument constructor creates a field with a default value,
		 * which is necessary for the BUILDER_FIELD macro that doesn't have a default value.
		 *
		 * Each constructor primes the cache (value) with an initial sample so the Value is usable before the owning
		 * effect calls sample() itself. provider is declared before value, so value(provider->sample()) is well-ordered.
		 */


		Value() : provider(std::make_shared<const ConstantProvider<T>>(T{})), value(provider->sample()) {}


		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
		Value(U&& constant)
			: provider(std::make_shared<const ConstantProvider<T>>(T(std::forward<U>(constant)))),
			  value(provider->sample()) {}


		/**
		 * Explicitly hold a provider.
		 */
		Value(std::shared_ptr<const Provider<T>> p) : provider(std::move(p)), value(provider->sample()) {}

		// Draw a fresh value from the provider and cache it in `value`. Non-const because it re-rolls (a random provider
		// returns a new draw), which is a mutation of this Value's cache - not a lazy const fill. The owning effect
		// calls this at lifecycle boundaries (its constructor and reset()); calculate() then just reads `value`. For a
		// constant provider this is a no-op that re-stores the same value.
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
} // namespace ledpipelines
