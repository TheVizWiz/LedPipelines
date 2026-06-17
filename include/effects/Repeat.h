#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Repeat : WrapperEffect {
		int numRepeats;
		float repeatDistance;
		BlendingMode blendingMode;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<Repeat> {
			BUILDER_FIELD_DEFAULT(int, numRepeats, 0);
			BUILDER_FIELD(float, repeatDistance);
			BUILDER_FIELD_DEFAULT(BlendingMode, blendingMode, BlendingMode::NORMAL);

			Builder(float repeatDistance) : _repeatDistance(repeatDistance) {};

			Repeat *build() override {
				return new Repeat(
					_stage,
					_numRepeats,
					_repeatDistance,
					_blendingMode
				);
			}
		};

		protected:
			Repeat(
				LedPipelineStage *stage,
				int numRepeats,
				float repeatDistance,
				BlendingMode blendingMode
			);
	};
}
