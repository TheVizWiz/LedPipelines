#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Flip : public WrapperEffect {
		// Bounds of the mirrored region. Either order (or a random draw where min > max) is fine.
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

			Flip* create() override {
				return new Flip(buildInner(), _minIndex, _maxIndex);
			}
		};

	private:
		Flip(LedPipelineStage* stage, Value<long> minIndex, Value<long> maxIndex);
	};
}
