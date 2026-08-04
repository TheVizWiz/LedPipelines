#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Wait : public LedPipelineStage, TimedEffect {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<Wait, Builder>, TimedEffect::Builder<Builder> {
			// runtimeMs is a Value<unsigned long>: a plain duration converts implicitly, or pass a RandomRange for a
			// randomized wait (this replaces the old RandomWait).
			explicit Builder(Value<unsigned long> runtimeMs) : TimedEffect::Builder<Builder>(std::move(runtimeMs)) {};

			Wait* build() override {
				return applyTiming(new Wait(_runtimeMs, _blendingMode));
			}
		};

	private:
		explicit Wait(Value<unsigned long> runtimeMs, BlendingMode blendingMode = BlendingMode::NORMAL);
	};
} // namespace ledpipelines::effects
