#pragma once


#include "LedPipelineUtils.h"
#include "BaseLedPipeline.h"
#include "effects/LedEffects.h"


namespace ledpipelines {
void initialize() {
    TemporaryLedData::initialize();
}


int ledCount() {
    return TemporaryLedData::size;
}
}