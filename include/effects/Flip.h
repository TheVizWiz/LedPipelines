#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Flip : public WrapperEffect {
		long minIndex;
		long maxIndex;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<Flip, Builder> {
			BUILDER_FIELD_DEFAULT(long, minIndex, 0);
			BUILDER_FIELD(long, maxIndex);

			explicit Builder(const long maxIndex) {
				this->_maxIndex = maxIndex;
			}

			Flip *build() override {
				return new Flip(_stage, _minIndex, _maxIndex);
			}
		};

		private:
			Flip(LedPipelineStage *stage, long minIndex, long maxIndex);
	};
}
