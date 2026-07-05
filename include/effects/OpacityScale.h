#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct OpacityScale : public WrapperEffect {
		uint8_t maxOpacity;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<OpacityScale, Builder> {
			BUILDER_FIELD_DEFAULT(uint8_t, maxOpacity, 0xFF);

			explicit Builder(const uint8_t maxOpacity) : _maxOpacity(maxOpacity) {}

			OpacityScale *build() override {
				return new OpacityScale(buildInner(), _maxOpacity);
			}
		};

		private:
			OpacityScale(LedPipelineStage *stage, uint8_t maxOpacity);
	};
}
