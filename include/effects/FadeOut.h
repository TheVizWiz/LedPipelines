#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct FadeOut : public LedPipelineStage, TimedEffect {
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<FadeOut, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			Builder(const unsigned long runtimeMs) : TimedEffect::Builder<Builder>(runtimeMs) {};

			FadeOut *create() override { return new FadeOut(_runtimeMs, _smoothingFunction, _blendingMode); }
		};

		private:
			FadeOut(
				unsigned long runtimeMs,
				SmoothingFunction smoothingFunction,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};

	struct RandomFadeOut : public LedPipelineStage, RandomTimedEffect {
		struct Builder : LedPipelineStage::Builder<RandomFadeOut, Builder>, RandomTimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);

			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomFadeOut *create() override {
				return new RandomFadeOut(
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction,
					_smoothingFunction,
					_blendingMode
				);
			}
		};

		SmoothingFunction smoothingFunction;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		private:
			RandomFadeOut(
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction,
				SmoothingFunction smoothingFunction,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};
}
