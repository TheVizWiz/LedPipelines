#pragma once

#include <functional>  // std::function - holds the warp (built-in preset or user-supplied)

namespace ledpipelines {
	/**
	 * Skews a uniform random draw into a non-uniform distribution.
	 *
	 * A SamplingFunction is built around a "warp": a pure function that maps a uniform draw x in [0, 1] to a value in
	 * [0, 1], reshaping where samples land. Density is high where the warp is flat and low where it is steep. UNIFORM is
	 * the identity (f(x) = x); the other presets bias toward the center or the edges. operator() draws a uniform value,
	 * runs it through the warp, then rescales the result into [min, max].
	 *
	 * Users can supply their OWN warp (any callable float(float), including lambdas with captures) instead of a preset,
	 * so custom distributions do not require editing this class:
	 *
	 *     effect.samplingFunction([](float x) { return x * x; });          // bias toward min
	 *     effect.samplingFunction(SamplingFunction::CENTERED);             // built-in preset
	 *
	 * A well-formed warp maps [0, 1] onto [0, 1] and is monotonic non-decreasing, so outputs stay within [min, max]; the
	 * library does not clamp, so a warp that leaves that range yields out-of-range samples by design.
	 */
	class SamplingFunction {
	public:
		/** A warp maps a uniform draw x in [0, 1] to a skewed value in [0, 1]. */
		using Warp = std::function<float(float)>;

		enum SamplingFunction_ {
			UNIFORM,
			CENTERED,
			EDGES,
		};

		/** Build from a preset (implicit, so existing call sites passing SamplingFunction::UNIFORM keep working). */
		SamplingFunction(SamplingFunction_ preset);

		/** Build from a user-supplied warp (implicit, so a lambda can be passed straight to a builder setter). */
		SamplingFunction(Warp warp) : warp(std::move(warp)) {}

#ifdef LEDPIPELINES_SERIALIZATION
		// The preset's stable string name for serialization, or "custom" for a user-supplied warp. Unlike
		// SmoothingFunction, a SamplingFunction can be built from an arbitrary lambda whose identity is erased into the
		// std::function, so it is not always reconstructible; the preset name is retained (see the preset ctor) only
		// when built from a preset. A "custom" warp serializes as a marker and rebuilds as UNIFORM.
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
		// Retained purely for serialization: which preset this was built from, and whether it was built from a preset at
		// all (vs. a custom warp). Set by the two constructors. Guarded so a non-serializing build carries neither the
		// extra byte nor the bool.
		SamplingFunction_ presetValue = UNIFORM;
		bool builtFromPreset = false;
#endif
	};
} // namespace ledpipelines
