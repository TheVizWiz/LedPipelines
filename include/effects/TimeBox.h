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

	struct RandomTimeBox : public WrapperEffect, RandomTimedEffect {
		void reset() override;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : WrapperEffect::Builder<RandomTimeBox, Builder>, RandomTimedEffect::Builder<Builder> {
			explicit Builder(const unsigned long maxRuntimeMs) : RandomTimedEffect::Builder<Builder>(maxRuntimeMs) {};

			RandomTimeBox* build() override {
				return applyTiming(
					new RandomTimeBox(buildInner(), _minRuntimeMs, _maxRuntimeMs, _samplingFunction)
				);
			}
		};

	protected:
		RandomTimeBox(
			LedPipelineStage* stage,
			unsigned long minRuntimeMs,
			unsigned long maxRuntimeMs,
			SamplingFunction samplingFunction
		);
	};
} // namespace ledpipelines::effects
