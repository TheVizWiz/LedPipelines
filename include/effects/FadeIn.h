#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	// FadeIn wraps an inner effect and ramps its opacity from 0 up to full over runtimeMs, then becomes a transparent
	// pass-through. Like OpacityGradient/OpacityScale it is a WrapperEffect that multiplies the inner effect's opacity
	// in place; it does NOT terminate on its own timer (that would make the wrapped content vanish once the ramp
	// finished), so its done-state defers to the inner effect.
	struct FadeIn : public WrapperEffect, TimedEffect {
		// The fade curve: a Value<SmoothingFunction> caching its own draw (see Value), like every other input. A plain
		// preset is a constant; a RandomSet gives a randomized curve re-rolled on reset(). calculate() reads its .value.
		Value<SmoothingFunction> smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<FadeIn, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			// runtimeMs is a Value<unsigned long>: a plain duration converts implicitly, or pass a RandomRange for a
			// randomized ramp (this replaces the old RandomFadeIn).
			explicit Builder(Value<unsigned long> runtimeMs) : TimedEffect::Builder<Builder>(std::move(runtimeMs)) {};

			FadeIn* build() override {
				return applyTiming(new FadeIn(buildInner(), _runtimeMs, _smoothingFunction, _blendingMode));
			}
		};

	private:
		FadeIn(
			LedPipelineStage* stage,
			Value<unsigned long> runtimeMs,
			Value<SmoothingFunction> smoothingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
} // namespace ledpipelines::effects
