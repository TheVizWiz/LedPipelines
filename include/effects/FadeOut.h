#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	// Ramps the inner effect's opacity from full down to 0 over runtimeMs. Unlike FadeIn, it terminates on its own
	// timer: DONE once the ramp completes, or earlier if the inner effect finishes first.
	struct FadeOut : public WrapperEffect, TimedEffect {
		Value<SmoothingFunction> smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		TimedEffect* timedEffect() override {
			return this;
		}

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<FadeOut, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			Builder(Value<unsigned long> runtimeMs) : TimedEffect::Builder<Builder>(std::move(runtimeMs)) {};

			FadeOut* create() override {
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
}
