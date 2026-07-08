#pragma once


#include "BaseEffect.h"
#include "LedPipelineUtils.h"

namespace ledpipelines::effects {
	struct Wait : public LedPipelineStage, TimedEffect {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<Wait, Builder>, TimedEffect::Builder<Builder> {
			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder<Builder>(runtimeMs) {};

			Wait* build() override {
				return applyTiming(new Wait(_runtimeMs, _blendingMode));
			}
		};

	private:
		explicit Wait(unsigned long runtimeMs, BlendingMode blendingMode = BlendingMode::NORMAL);
	};


	struct RandomWaitEffect : public LedPipelineStage, RandomTimedEffect {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<RandomWaitEffect, Builder>, RandomTimedEffect::Builder<Builder> {
			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomWaitEffect* build() override {
				return applyTiming(
					new RandomWaitEffect(_minRuntimeMs, _maxRuntimeMs, _samplingFunction, _blendingMode)
				);
			}
		};

	private:
		RandomWaitEffect(
			unsigned long minRuntimeMs,
			unsigned long maxRuntimeMs,
			SamplingFunction samplingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
} // namespace ledpipelines::effects
