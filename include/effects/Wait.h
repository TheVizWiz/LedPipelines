#pragma once


#include "BaseEffect.h"
#include "LedPipelineUtils.h"

namespace ledpipelines::effects {
	class Wait : public LedPipelineStage, TimedEffect {
	public:
		using Config = TimedEffect::Config;

		Wait(const Config &config);

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;
	};


	class RandomWaitEffect : public LedPipelineStage, RandomTimedEffect {
	public:
		using Config = RandomTimedEffect::Config;

		RandomWaitEffect(const Config &config);

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;
	};
}