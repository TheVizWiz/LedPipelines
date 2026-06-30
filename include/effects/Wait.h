#pragma once


#include "BaseEffect.h"
#include "LedPipelineUtils.h"

namespace ledpipelines::effects {
	struct Wait : public LedPipelineStage, TimedEffect {
		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<Wait>, TimedEffect::Builder {
			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder(runtimeMs) {};

			Wait *build() override {
				return new Wait(_runtimeMs);
			}
		};

		private:
			explicit Wait(unsigned long runtimeMs);
	};


	struct RandomWaitEffect : public LedPipelineStage, RandomTimedEffect {
		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<RandomWaitEffect>, RandomTimedEffect::Builder {
			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder(maxRuntimeMs) {};

			RandomWaitEffect *build() override {
				return new RandomWaitEffect(
					_minRuntimeMs,
					_maxRuntimeMs,
					_samplingFunction
				);
			}
		};

		private:
			RandomWaitEffect(
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction
			);
	};
}
