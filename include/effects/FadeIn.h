#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct FadeIn : public LedPipelineStage, TimedEffect {
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<FadeIn>, TimedEffect::Builder {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder(runtimeMs) {};

			FadeIn *build() override {
				return new FadeIn(
					_runtimeMs,
					_smoothingFunction
				);
			}
		};

		private:
			FadeIn(
				unsigned long runtimeMs,
				SmoothingFunction smoothingFunction
			);
	};

	struct RandomFadeIn : public LedPipelineStage, RandomTimedEffect {
		SmoothingFunction smoothingFunction;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<RandomFadeIn>, RandomTimedEffect::Builder {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder(maxRuntimeMs) {};

			RandomFadeIn *build() override {
				return new RandomFadeIn(
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction,
					_smoothingFunction

				);
			};
		};

		private:
			RandomFadeIn(
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction,
				SmoothingFunction smoothingFunction
			);
	};
}
