#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct FadeIn : public LedPipelineStage, TimedEffect {
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<FadeIn, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder<Builder>(runtimeMs) {};

			FadeIn *create() override {
				return new FadeIn(
					_runtimeMs,
					_smoothingFunction,
					_blendingMode
				);
			}
		};

		private:
			FadeIn(
				unsigned long runtimeMs,
				SmoothingFunction smoothingFunction,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};

	struct RandomFadeIn : public LedPipelineStage, RandomTimedEffect {
		SmoothingFunction smoothingFunction;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<RandomFadeIn, Builder>, RandomTimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomFadeIn *create() override {
				return new RandomFadeIn(
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
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction,
				SmoothingFunction smoothingFunction,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};
}
