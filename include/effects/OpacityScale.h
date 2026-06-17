#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	class OpacityScale : public WrapperEffect {
	public:
		uint8_t maxOpacity;

		OpacityScale(LedPipelineStage *stage, uint8_t maxOpacity);

		void calculate(float startIndex, TemporaryLedData &tempData) override;
	};
}