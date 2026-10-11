#pragma once

// Free-function JSON field formatters, shared by everything that serializes. LedPipelineStage exposes these as
// protected member helpers (thin forwarders) so effect toJson() bodies read as jsonField(...); the standalone timing
// mixin (TimedEffect in BaseEffect.h), which does NOT inherit LedPipelineStage and so cannot reach
// its protected members, call these free functions directly. Keeping the formatting in one place means both paths
// produce identical JSON.
//
// Compiled only under LEDPIPELINES_SERIALIZATION (like the rest of the serialization surface); the header is included
// unconditionally but its body is empty otherwise, so a non-serializing build pulls in nothing.
#ifdef LEDPIPELINES_SERIALIZATION

#include <cstdio>       // snprintf
#include <functional>
#include <string>
#include <type_traits>  // is_integral / is_signed - the integral jsonValue template

#include "Color.h"                   // RGBA, HSVA
#include "enums/BlendingMode.h"       // BlendingMode + blendingModeName
#include "enums/SamplingFunction.h"   // SamplingFunction
#include "enums/SmoothingFunction.h"  // SmoothingFunction

namespace ledpipelines::serialization {
	// Each jsonField prepends ",\"name\" : value". Callers open an object with a leading, comma-less field (jsonHeader
	// / jsonStateHeader) and then append these, so the commas fall between fields exactly once.

	inline std::string jsonField(const std::string& name, const std::string& value) {
		return ",\"" + name + "\" : \"" + value + "\"";
	}

	inline std::string jsonField(const std::string& name, int value) {
		// Format the number into a fixed scratch buffer, then append. The buffer bounds only the single number's text
		// (an int is at most 11 chars + sign + NUL), never the returned string; snprintf always NUL-terminates and
		// never overflows. 32 bytes is comfortably larger than any int/float token.
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", value);
		return (",\"" + name + "\" : ").append(buf);
	}

	inline std::string jsonField(const std::string& name, float value) {
		// See the int overload. "%.9g" round-trips a 32-bit float (9 significant digits) without the trailing-zero
		// verbosity of std::to_string, and is locale-independent.
		char buf[32];
		snprintf(buf, sizeof(buf), "%.9g", (double)value);
		return (",\"" + name + "\" : ").append(buf);
	}

	inline std::string jsonField(const std::string& name, bool value) {
		return ",\"" + name + (value ? "\" : true" : "\" : false");
	}

	// Enum-typed fields serialize as their stable string name (see each enum's name()/blendingModeName). Routed through
	// the string overload so they are quoted like any other string value.
	inline std::string jsonField(const std::string& name, SmoothingFunction value) {
		return jsonField(name, std::string(value.name()));
	}

	inline std::string jsonField(const std::string& name, const SamplingFunction& value) {
		return jsonField(name, std::string(value.name()));
	}

	inline std::string jsonField(const std::string& name, BlendingMode value) {
		return jsonField(name, std::string(blendingModeName(value)));
	}

	// A field whose value is already a JSON fragment (a nested stage's toJson(), or an object built below) - emitted
	// WITHOUT the surrounding quotes the string overload adds, so the nested JSON is not corrupted into a quoted string.
	inline std::string jsonRawField(const std::string& name, const std::string& rawJson) {
		return ",\"" + name + "\" : " + rawJson;
	}

	// Opens a Value<T> provider object with its discriminator - the value analog of jsonHeader() for stages. Every
	// serialized Value<T> is an object tagged with a "provider" naming the provider kind ("constant", "randomColor",
	// "randomHsvColor", "randomColorSet", ...); the remaining fields are that provider's own params. No leading comma:
	// this is the FIRST field of the object, so provider params appended after (jsonField / jsonRawField) add theirs.
	// Usage: "{" + jsonProvider("constant") + jsonRawField("value", ...) + "}".
	inline std::string jsonProvider(const std::string& kind) {
		return "\"provider\" : \"" + kind + "\"";
	}

	// jsonValue(x): the bare JSON value for x (no key), used where a value stands alone rather than as a named field -
	// e.g. inside an array, or a ConstantProvider<T>::describeValue() rendering its wrapped value. The jsonField(name,
	// x) overloads for object-typed values (RGBA, HSVA) are defined in terms of these.

	// Bare bool, for ConstantProvider<bool>::describeValue() (e.g. Spawner's keepOldOnSpawn). A non-template exact match,
	// so it wins over the integral template below (which excludes bool) - a Value<bool> serializes as true/false, not 1/0.
	inline std::string jsonValue(bool value) {
		return value ? "true" : "false";
	}

	// Bare integer, for ConstantProvider<T>::describeValue() over any integral T (int, unsigned long, long, size_t,
	// uint16_t, uint8_t, ...). A single template rather than one overload per width, so every builder field of integral
	// type resolves unambiguously. bool is excluded (handled above); printed via long long / unsigned long long to
	// cover the full range. Formatted into a fixed scratch buffer like the jsonField numeric overloads.
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

	// Enum-typed bare values, for ConstantProvider<SmoothingFunction/SamplingFunction/BlendingMode>::describeValue()
	// (every builder field is now a Value<T>, so these T's get a ConstantProvider too). Each serializes as its stable
	// string name, quoted - matching the corresponding jsonField overload above.
	inline std::string jsonValue(SmoothingFunction value) {
		return "\"" + std::string(value.name()) + "\"";
	}

	inline std::string jsonValue(const SamplingFunction& value) {
		return "\"" + std::string(value.name()) + "\"";
	}

	inline std::string jsonValue(BlendingMode value) {
		return "\"" + std::string(blendingModeName(value)) + "\"";
	}

	// A color's bare value: {"r":255,"g":0,"b":0,"a":255}.
	inline std::string jsonValue(const RGBA& value) {
		std::string obj = "{";
		obj += jsonField("r", (int)value.r).substr(1);  // substr(1) drops the leading comma so the object opens cleanly
		obj += jsonField("g", (int)value.g);
		obj += jsonField("b", (int)value.b);
		obj += jsonField("a", (int)value.a);
		obj += "}";
		return obj;
	}

	// A floating-point HSVA color's bare value: {"h":..,"s":..,"v":..,"a":..}.
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
	 * A callable's bare value. Callbacks (e.g. a Hook's callback) have no JSON representation, so they serialize as
	 * null.
	 */
	template <typename R, typename... Args>
	inline std::string jsonValue(const std::function<R(Args...)>&) {
		return "null";
	}

	// A color serializes as an object of its four channels, e.g. "color" : {"r":255,"g":0,"b":0,"a":255}. Emitted as a
	// raw (unquoted) JSON object.
	inline std::string jsonField(const std::string& name, const RGBA& value) {
		return jsonRawField(name, jsonValue(value));
	}

	// A floating-point HSVA color: "grad" : {"h":..,"s":..,"v":..,"a":..}.
	inline std::string jsonField(const std::string& name, const HSVA& value) {
		return jsonRawField(name, jsonValue(value));
	}
} // namespace ledpipelines::serialization

#endif // LEDPIPELINES_SERIALIZATION
