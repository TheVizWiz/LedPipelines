#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Loop : public WrapperEffect {
		size_t numLoops = 0;
		size_t currentNumLoops = 0;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : WrapperEffect::Builder<Loop, Builder> {
			BUILDER_FIELD_DEFAULT(size_t, numLoops, 0);

			Builder() {};

			Loop* build() override {
				return new Loop(buildInner(), _numLoops);
			};
		};

	private:
		explicit Loop(LedPipelineStage* stage, size_t numLoops);
	};
} // namespace ledpipelines::effects
