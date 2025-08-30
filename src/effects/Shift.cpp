#include "effects/Shift.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Shift::Shift(BaseLedPipelineStage *stage, float offset)
        : WrapperEffect(stage),
          offset(offset) {
    this->blendingMode = stage->blendingMode;
}


void Shift::calculate(float startIndex, TemporaryLedData &tempData) {
    this->stage->calculate(startIndex + offset, tempData);
}

Shift::Shift(BaseLedPipelineStage *stage, const Shift::Config &config)
        : Shift(stage, config.offset) {}
