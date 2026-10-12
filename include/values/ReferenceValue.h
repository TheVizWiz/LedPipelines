#pragma once

#include <memory>

#include "Value.h"

#ifdef LEDPIPELINES_SERIALIZATION
#include <string>
#include "serialization/JsonFields.h"
#endif

// Values that read an externally stored variable. The referenced variable must outlive every effect built from the
// value; temporaries are rejected.
//   - ReferenceValue copies the variable each time it is sampled: once when constructed and again on every reset() of
//     the owning effect. Reads in between return that cached copy.
//   - LiveReferenceValue additionally reads the variable on every read (Value::get()), so effects see changes
//     immediately; each read also refreshes the cached value.
//
// T must MATCH the target field's Value<T>; CTAD deduces it from the referenced variable's type.
namespace ledpipelines {
	namespace providers {
		template <typename T>
		struct ReferenceProvider : Provider<T> {
			const T* reference;

			explicit ReferenceProvider(const T& reference) : reference(&reference) {}

			T sample() const override {
				return *reference;
			}

#ifdef LEDPIPELINES_SERIALIZATION
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("reference") + "}";
			}
#endif
		};

		template <typename T>
		struct LiveReferenceProvider : ReferenceProvider<T> {
			using ReferenceProvider<T>::ReferenceProvider;

			const T& get(T& cached) const override {
				cached = *this->reference;
				return cached;
			}

#ifdef LEDPIPELINES_SERIALIZATION
			std::string describeValue() const override {
				using namespace serialization;
				return "{" + jsonProvider("liveReference") + "}";
			}
#endif
		};
	}

	template <typename T>
	struct ReferenceValue : Value<T> {
		explicit ReferenceValue(const T& reference)
			: Value<T>(std::make_shared<const providers::ReferenceProvider<T>>(reference)) {}

		ReferenceValue(const T&&) = delete;
	};

	template <typename T>
	struct LiveReferenceValue : Value<T> {
		explicit LiveReferenceValue(const T& reference)
			: Value<T>(std::make_shared<const providers::LiveReferenceProvider<T>>(reference)) {}

		LiveReferenceValue(const T&&) = delete;
	};
}
