#pragma once

// Free-function JSON field formatters, shared by everything that serializes. LedPipelineStage exposes these as
// protected member helpers (thin forwarders) so effect toJson() bodies read as jsonField(...); the standalone timing
// mixins (TimedEffect / RandomTimedEffect in BaseEffect.h), which do NOT inherit LedPipelineStage and so cannot reach
// its protected members, call these free functions directly. Keeping the formatting in one place means both paths
// produce identical JSON.
//
// Compiled only under LEDPIPELINES_SERIALIZATION (like the rest of the serialization surface); the header is included
// unconditionally but its body is empty otherwise, so a non-serializing build pulls in nothing.
#ifdef LEDPIPELINES_SERIALIZATION

#include <cstdio>   // snprintf
#include <string>

#include "Color.h"                   // RGBA, FHSVA
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

	// A color serializes as an object of its four channels, e.g. "color" : {"r":255,"g":0,"b":0,"a":255}. Emitted as a
	// raw (unquoted) JSON object. substr(1) drops each helper's leading comma so the first field opens cleanly.
	inline std::string jsonField(const std::string& name, const RGBA& value) {
		std::string obj = "{";
		obj += jsonField("r", (int)value.r).substr(1);
		obj += jsonField("g", (int)value.g);
		obj += jsonField("b", (int)value.b);
		obj += jsonField("a", (int)value.a);
		obj += "}";
		return jsonRawField(name, obj);
	}

	// A floating-point HSVA color serializes as {"h":..,"s":..,"v":..,"a":..}.
	inline std::string jsonField(const std::string& name, const FHSVA& value) {
		std::string obj = "{";
		obj += jsonField("h", value.h).substr(1);
		obj += jsonField("s", value.s);
		obj += jsonField("v", value.v);
		obj += jsonField("a", (int)value.a);
		obj += "}";
		return jsonRawField(name, obj);
	}
} // namespace ledpipelines::serialization

#endif // LEDPIPELINES_SERIALIZATION
