#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Repeat : WrapperEffect {
		int numRepeats;
		float repeatDistance;
		// How the repeated copies stack onto each other within this effect. Distinct from the inherited blendingMode,
		// which controls how the whole Repeat result blends into its parent (a wrapper inherits that from its child).
		BlendingMode repeatBlendingMode;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<Repeat, Builder> {
			BUILDER_FIELD_DEFAULT(int, numRepeats, 0);
			BUILDER_FIELD(float, repeatDistance);
			BUILDER_FIELD_DEFAULT(BlendingMode, repeatBlendingMode, BlendingMode::NORMAL);

			Builder(float repeatDistance) : _repeatDistance(repeatDistance) {};

			Repeat *create() override {
				return new Repeat(
					_stage,
					_numRepeats,
					_repeatDistance,
					_repeatBlendingMode
				);
			}
		};

		protected:
			Repeat(
				LedPipelineStage *stage,
				int numRepeats,
				float repeatDistance,
				BlendingMode repeatBlendingMode
			);
	};
}
