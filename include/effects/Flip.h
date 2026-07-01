#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Flip : public WrapperEffect {
		long min;
		long max;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<Flip, Builder> {
			BUILDER_FIELD_DEFAULT(long, min, 0);
			BUILDER_FIELD(long, max);

			explicit Builder(const long max) {
				this->_max = max;
			}

			Flip *build() override {
				return new Flip(_stage, _min, _max);
			}
		};

		private:
			Flip(LedPipelineStage *stage, long min, long max);
	};
}
