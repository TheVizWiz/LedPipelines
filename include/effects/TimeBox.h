#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct TimeBox : public WrapperEffect, TimedEffect {
		void reset() override;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : WrapperEffect::Builder<TimeBox, Builder>, TimedEffect::Builder<Builder> {
			explicit Builder(const unsigned long runtimeMs) : TimedEffect::Builder<Builder>(runtimeMs) {};

			TimeBox* build() override {
				return applyTiming(new TimeBox(buildInner(), _runtimeMs));
			}
		};

	protected:
		TimeBox(LedPipelineStage* stage, unsigned long runtimeMs);
	};

	struct RandomTimeBoxedEffect : public WrapperEffect, RandomTimedEffect {
		void reset() override;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : WrapperEffect::Builder<RandomTimeBoxedEffect, Builder>, RandomTimedEffect::Builder<Builder> {
			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomTimeBoxedEffect* build() override {
				return applyTiming(
					new RandomTimeBoxedEffect(buildInner(), _minRuntimeMs, _maxRuntimeMs, _samplingFunction)
				);
			}
		};

	protected:
		RandomTimeBoxedEffect(
			LedPipelineStage* stage,
			unsigned long minRuntimeMs,
			unsigned long maxRuntimeMs,
			SamplingFunction samplingFunction
		);
	};
} // namespace ledpipelines::effects
