#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct TimeBox : public WrapperEffect, TimedEffect {
		void reset() override;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<TimeBox>, TimedEffect::Builder {
			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder(runtimeMs) {};

			TimeBox *build() override {
				return new TimeBox(_stage, _runtimeMs);
			}
		};

		protected:
			TimeBox(LedPipelineStage *stage, unsigned long runtimeMs);
	};

	struct RandomTimeBoxedEffect : public WrapperEffect, RandomTimedEffect {
		void reset() override;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<RandomTimeBoxedEffect>, RandomTimedEffect::Builder {
			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder(maxRuntimeMs) {};

			RandomTimeBoxedEffect *build() override {
				return new RandomTimeBoxedEffect(
					_stage,
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction
				);
			}
		};

		protected:
			RandomTimeBoxedEffect(
				LedPipelineStage *stage,
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction
			);
	};
}
