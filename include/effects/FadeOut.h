#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	// FadeOut wraps an inner effect and ramps its opacity from full down to 0 over runtimeMs. Like OpacityGradient it
	// is a WrapperEffect that multiplies the inner effect's opacity in place. Unlike FadeIn, it DOES terminate on its
	// own timer: fading content out to nothing is the whole point, so the wrapper reports DONE once the ramp completes
	// (or earlier if the inner effect finishes first).
	struct FadeOut : public WrapperEffect, TimedEffect {
		// The fade curve: a Value<SmoothingFunction> caching its own draw (see Value), like every other input. A plain
		// preset is a constant; a RandomSet gives a randomized curve re-rolled on reset(). calculate() reads its .value.
		Value<SmoothingFunction> smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<FadeOut, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			// runtimeMs is a Value<unsigned long>: a plain duration converts implicitly, or pass a RandomRange for a
			// randomized ramp (this replaces the old RandomFadeOut).
			Builder(Value<unsigned long> runtimeMs) : TimedEffect::Builder<Builder>(std::move(runtimeMs)) {};

			FadeOut* build() override {
				return applyTiming(new FadeOut(buildInner(), _runtimeMs, _smoothingFunction, _blendingMode));
			}
		};

	private:
		FadeOut(
			LedPipelineStage* stage,
			Value<unsigned long> runtimeMs,
			Value<SmoothingFunction> smoothingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
} // namespace ledpipelines::effects
