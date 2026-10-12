#pragma once

// JSON formatters shared by LedPipelineStage (via protected forwarders) and the timing mixins, which don't inherit it.
// Included unconditionally; empty unless LEDPIPELINES_SERIALIZATION is defined.
#ifdef LEDPIPELINES_SERIALIZATION

#include <cstdio>
#include <functional>
#include <string>
#include <type_traits>

#include "LedColor.h"
#include "enums/BlendingMode.h"
#include "enums/SamplingFunction.h"
#include "enums/SmoothingFunction.h"

namespace ledpipelines::serialization {
	// Each jsonField prepends ",\"name\" : value". Callers open an object with a comma-less first field (jsonHeader /
	// jsonStateHeader) and then append these.
	inline std::string jsonField(const std::string& name, const std::string& value) {
		return ",\"" + name + "\" : \"" + value + "\"";
	}

	inline std::string jsonField(const std::string& name, int value) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", value);
		return (",\"" + name + "\" : ").append(buf);
	}

	inline std::string jsonField(const std::string& name, float value) {
		// "%.9g" round-trips a 32-bit float without std::to_string's trailing zeros.
		char buf[32];
		snprintf(buf, sizeof(buf), "%.9g", (double)value);
		return (",\"" + name + "\" : ").append(buf);
	}

	inline std::string jsonField(const std::string& name, bool value) {
		return ",\"" + name + (value ? "\" : true" : "\" : false");
	}

	// Enum-typed fields serialize as their stable string name.
	inline std::string jsonField(const std::string& name, SmoothingFunction value) {
		return jsonField(name, std::string(value.name()));
	}

	inline std::string jsonField(const std::string& name, const SamplingFunction& value) {
		return jsonField(name, std::string(value.name()));
	}

	inline std::string jsonField(const std::string& name, BlendingMode value) {
		return jsonField(name, std::string(blendingModeName(value)));
	}

	// For a value that is already a JSON fragment (e.g. a nested toJson()); emitted unquoted.
	inline std::string jsonRawField(const std::string& name, const std::string& rawJson) {
		return ",\"" + name + "\" : " + rawJson;
	}

	// First field of a serialized Value<T> object, naming the provider kind ("constant", "randomColor", ...). No
	// leading comma. Usage: "{" + jsonProvider("constant") + jsonRawField("value", ...) + "}".
	inline std::string jsonProvider(const std::string& kind) {
		return "\"provider\" : \"" + kind + "\"";
	}

	// jsonValue(x): the bare JSON value for x (no key), e.g. for ConstantProvider<T>::describeValue().
	inline std::string jsonValue(bool value) {
		return value ? "true" : "false";
	}

	// One template for all integral widths so overload resolution is never ambiguous. bool is excluded so it
	// serializes as true/false, not 1/0.
	template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
	inline std::string jsonValue(T value) {
		char buf[32];
		if (std::is_signed_v<T>) {
			snprintf(buf, sizeof(buf), "%lld", (long long)value);
		} else {
			snprintf(buf, sizeof(buf), "%llu", (unsigned long long)value);
		}
		return std::string(buf);
	}

	inline std::string jsonValue(float value) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%.9g", (double)value);
		return std::string(buf);
	}

	inline std::string jsonValue(SmoothingFunction value) {
		return "\"" + std::string(value.name()) + "\"";
	}

	inline std::string jsonValue(const SamplingFunction& value) {
		return "\"" + std::string(value.name()) + "\"";
	}

	inline std::string jsonValue(BlendingMode value) {
		return "\"" + std::string(blendingModeName(value)) + "\"";
	}

	inline std::string jsonValue(const RGBA& value) {
		std::string obj = "{";
		obj += jsonField("r", (int)value.r).substr(1);  // drop the leading comma
		obj += jsonField("g", (int)value.g);
		obj += jsonField("b", (int)value.b);
		obj += jsonField("a", (int)value.a);
		obj += "}";
		return obj;
	}

	inline std::string jsonValue(const HSVA& value) {
		std::string obj = "{";
		obj += jsonField("h", value.h).substr(1);
		obj += jsonField("s", value.s);
		obj += jsonField("v", value.v);
		obj += jsonField("a", (int)value.a);
		obj += "}";
		return obj;
	}

	/**
	 * Callbacks have no JSON representation, so they serialize as null.
	 */
	template <typename R, typename... Args>
	inline std::string jsonValue(const std::function<R(Args...)>&) {
		return "null";
	}

	inline std::string jsonField(const std::string& name, const RGBA& value) {
		return jsonRawField(name, jsonValue(value));
	}

	inline std::string jsonField(const std::string& name, const HSVA& value) {
		return jsonRawField(name, jsonValue(value));
	}
}

#endif // LEDPIPELINES_SERIALIZATION
