#pragma once

#include <functional>

namespace ledpipelines {
	/**
	 * Skews a uniform random draw into a non-uniform distribution via a "warp": a function mapping a uniform x in
	 * [0, 1] to [0, 1], then rescaled into [min, max]. Density is high where the warp is flat and low where it is steep.
	 * A preset or any custom callable can be used:
	 *
	 *     effect.samplingFunction([](float x) { return x * x; });          // bias toward min
	 *     effect.samplingFunction(SamplingFunction::CENTERED);             // built-in preset
	 *
	 * Results are not clamped, so a warp that leaves [0, 1] yields out-of-range samples.
	 */
	class SamplingFunction {
	public:
		using Warp = std::function<float(float)>;

		enum SamplingFunction_ {
			UNIFORM,
			CENTERED,
			EDGES,
		};

		SamplingFunction(SamplingFunction_ preset);

		/** Implicit so a lambda can be passed straight to a builder setter. */
		SamplingFunction(Warp warp) : warp(std::move(warp)) {}

#ifdef LEDPIPELINES_SERIALIZATION
		// "custom" for a user-supplied warp, which cannot be reconstructed and deserializes as UNIFORM.
		const char* name() const {
			if (!builtFromPreset) return "custom";
			switch (presetValue) {
			case UNIFORM: return "UNIFORM";
			case CENTERED: return "CENTERED";
			case EDGES: return "EDGES";
			}
			return "UNIFORM";
		}
#endif

		float operator()() const {
			return this->operator()(0, 1);
		}

		float operator()(float max) const {
			return this->operator()(0, max);
		};

		float operator()(float min, float max) const;

	private:
		Warp warp;

#ifdef LEDPIPELINES_SERIALIZATION
		SamplingFunction_ presetValue = UNIFORM;
		bool builtFromPreset = false;
#endif
	};
}
