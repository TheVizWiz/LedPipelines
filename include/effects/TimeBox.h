#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct TimeBox : public WrapperEffect, TimedEffect {
		void reset() override;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<TimeBox, Builder>, TimedEffect::Builder<Builder> {
			explicit Builder(Value<unsigned long> runtimeMs) : TimedEffect::Builder<Builder>(std::move(runtimeMs)) {};

			TimeBox* build() override {
				return applyTiming(new TimeBox(buildInner(), _runtimeMs));
			}
		};

	protected:
		TimeBox(LedPipelineStage* stage, Value<unsigned long> runtimeMs);
	};
}
