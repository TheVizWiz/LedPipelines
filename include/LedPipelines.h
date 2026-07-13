#pragma once


#include "BaseLedPipeline.h"
#include "LedOutput.h"
#include "TemporaryLedData.h"
#include "effects/LedEffects.h"


namespace ledpipelines {
	// Defined inline: these are free functions living in a header, so without `inline` every translation unit that
	// includes LedPipelines.h emits its own definition and linking two such units fails with duplicate-symbol errors.
	inline void initialize() {
		TemporaryLedData::initialize();
	}


	inline int ledCount() {
		return TemporaryLedData::size;
	}
} // namespace ledpipelines
