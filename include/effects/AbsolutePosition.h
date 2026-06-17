#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct AbsolutePosition : WrapperEffect {
		struct Builder : WrapperEffect::Builder<AbsolutePosition> {
			float position = 0;

			Builder(float position = 0);

			AbsolutePosition *build() override;
		};

		float position;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		private:
			explicit AbsolutePosition(LedPipelineStage *stage, float position = 0);
	};
}
