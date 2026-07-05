#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Shift : WrapperEffect {
		float offset;

		void calculate(float startIndex, TemporaryLedData &tempData) override;


		struct Builder : WrapperEffect::Builder<Shift, Builder> {
			BUILDER_FIELD(float, offset);

			Builder(float offset) : _offset(offset) {}

			Shift *create() override {
				return new Shift(buildInner(), _offset);
			}
		};

		protected:
			Shift(LedPipelineStage *stage, float offset);
	};
}
