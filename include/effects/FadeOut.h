#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	// FadeOut wraps an inner effect and ramps its opacity from full down to 0 over runtimeMs. Like OpacityGradient it is
	// a WrapperEffect that multiplies the inner effect's opacity in place. Unlike FadeIn, it DOES terminate on its own
	// timer: fading content out to nothing is the whole point, so the wrapper reports DONE once the ramp completes (or
	// earlier if the inner effect finishes first).
	struct FadeOut : public WrapperEffect, TimedEffect {
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : WrapperEffect::Builder<FadeOut, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			Builder(const unsigned long runtimeMs) : TimedEffect::Builder<Builder>(runtimeMs) {};

			FadeOut *build() override {
				return applyTiming(new FadeOut(buildInner(), _runtimeMs, _smoothingFunction, _blendingMode));
			}
		};

		private:
			FadeOut(
				LedPipelineStage *stage,
				unsigned long runtimeMs,
				SmoothingFunction smoothingFunction,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};

	struct RandomFadeOut : public WrapperEffect, RandomTimedEffect {
		struct Builder : WrapperEffect::Builder<RandomFadeOut, Builder>, RandomTimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);

			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomFadeOut *build() override {
				return applyTiming(new RandomFadeOut(
					buildInner(),
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction,
					_smoothingFunction,
					_blendingMode
				));
			}
		};

		SmoothingFunction smoothingFunction;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		private:
			RandomFadeOut(
				LedPipelineStage *stage,
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction,
				SmoothingFunction smoothingFunction,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};
}
