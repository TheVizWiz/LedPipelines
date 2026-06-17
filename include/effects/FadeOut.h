#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct FadeOut : public LedPipelineStage, TimedEffect {
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<FadeOut>, TimedEffect::Builder {
			BUILDER_FIELD_DEFAULT(
				SmoothingFunction,
				smoothingFunction,
				SmoothingFunction::SMOOTH_LINEAR
			);

			Builder(const unsigned long runtimeMs) : TimedEffect::Builder(runtimeMs) {};

			FadeOut *build() override { return new FadeOut(_runtimeMs, _smoothingFunction); }
		};

		private:
			FadeOut(
				unsigned long runtimeMs,
				SmoothingFunction smoothingFunction
			);
	};

	struct RandomFadeOut : public LedPipelineStage, RandomTimedEffect {
		struct Builder : LedPipelineStage::Builder<RandomFadeOut>, RandomTimedEffect::Builder {
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);

			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder(maxRuntimeMs) {};

			RandomFadeOut *build() override {
				return new RandomFadeOut(
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction,
					_smoothingFunction
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
				SmoothingFunction smoothingFunction
			);
	};
}

}
