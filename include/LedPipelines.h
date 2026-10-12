#pragma once

#include "BaseLedPipeline.h"
#include "LedOutput.h"
#include "TemporaryLedData.h"
#include "effects/LedEffects.h"

namespace ledpipelines {
	inline void initialize() {
		TemporaryLedData::initialize();
	}

	inline int ledCount() {
		return TemporaryLedData::size;
	}
}
