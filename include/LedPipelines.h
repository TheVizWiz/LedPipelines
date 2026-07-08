#pragma once


#include "BaseLedPipeline.h"
#include "LedPipelineUtils.h"
#include "effects/LedEffects.h"


namespace ledpipelines {
	void initialize() {
		TemporaryLedData::initialize();
	}


	int ledCount() {
		return TemporaryLedData::size;
	}
} // namespace ledpipelines
