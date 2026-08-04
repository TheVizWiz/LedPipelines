#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Flip : public WrapperEffect {
		// The mirror-region bounds: Value<long> recipes caching their own draw (see Value). Plain longs are constants; a
		// RandomRange/RandomSet re-rolls on reset(). calculate() reads .value and normalizes their order itself, so either
		// order (or a random draw where min > max) is fine.
		Value<long> minIndex;
		Value<long> maxIndex;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<Flip, Builder> {
			BUILDER_FIELD_DEFAULT(long, minIndex, 0);
			BUILDER_FIELD(long, maxIndex);

			explicit Builder(Value<long> maxIndex) {
				this->_maxIndex = std::move(maxIndex);
			}

			Flip* build() override {
				return new Flip(buildInner(), _minIndex, _maxIndex);
			}
		};

	private:
		Flip(LedPipelineStage* stage, Value<long> minIndex, Value<long> maxIndex);
	};
} // namespace ledpipelines::effects
