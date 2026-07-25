#pragma once


namespace ledpipelines {
	enum class BlendingMode {
		NORMAL,
		OVERWRITE,
		ADD,
		MULTIPLY,
		MASK,
	};

#ifdef LEDPIPELINES_SERIALIZATION
	// The mode's stable string name, emitted in serialized JSON. Kept next to the enum so the name<->value mapping has a
	// single source of truth (mirrors SmoothingFunction::name / SamplingFunction::name).
	inline const char* blendingModeName(BlendingMode mode) {
		switch (mode) {
		case BlendingMode::NORMAL: return "NORMAL";
		case BlendingMode::OVERWRITE: return "OVERWRITE";
		case BlendingMode::ADD: return "ADD";
		case BlendingMode::MULTIPLY: return "MULTIPLY";
		case BlendingMode::MASK: return "MASK";
		}
		return "NORMAL";
	}
#endif
}
