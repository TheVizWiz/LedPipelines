#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Shift : WrapperEffect {
		float offset;

		void calculate(float startIndex, TemporaryLedData &tempData) override;


		struct Builder : WrapperEffect::Builder<Shift> {
			BUILDER_FIELD(float, offset);

			Builder(float offset) : _offset(offset) {}

			Shift *build() override {
				return new Shift(_stage, _offset);
			}
		};

		protected:
			Shift(LedPipelineStage *stage, float offset);
	};
}
