#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	// FadeIn wraps an inner effect and ramps its opacity from 0 up to full over runtimeMs, then becomes a transparent
	// pass-through. Like OpacityGradient/OpacityScale it is a WrapperEffect that multiplies the inner effect's opacity
	// in place; it does NOT terminate on its own timer (that would make the wrapped content vanish once the ramp
	// finished), so its done-state defers to the inner effect.
	struct FadeIn : public WrapperEffect, TimedEffect {
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : WrapperEffect::Builder<FadeIn, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder<Builder>(runtimeMs) {};

			FadeIn* build() override {
				return applyTiming(new FadeIn(buildInner(), _runtimeMs, _smoothingFunction, _blendingMode));
			}
		};

	private:
		FadeIn(
			LedPipelineStage* stage,
			unsigned long runtimeMs,
			SmoothingFunction smoothingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};

	struct RandomFadeIn : public WrapperEffect, RandomTimedEffect {
		SmoothingFunction smoothingFunction;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : WrapperEffect::Builder<RandomFadeIn, Builder>, RandomTimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomFadeIn* build() override {
				return new RandomFadeIn(
					buildInner(),
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction,
					_smoothingFunction,
					_blendingMode
				);
			};
		};

	private:
		RandomFadeIn(
			LedPipelineStage* stage,
			unsigned long minRuntimeMs,
			unsigned long maxRuntimeMs,
			SamplingFunction samplingFunction,
			SmoothingFunction smoothingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
} // namespace ledpipelines::effects
