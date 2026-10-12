#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	// Ramps the inner effect's opacity from 0 up to full over runtimeMs, then becomes a transparent pass-through. It
	// does NOT terminate on its own timer (unless terminateOnComplete is set), so its done-state defers to the inner.
	struct FadeIn : public WrapperEffect, TimedEffect {
		Value<SmoothingFunction> smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<FadeIn, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

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
}
