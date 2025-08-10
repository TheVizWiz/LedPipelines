#include "effects/OffsetEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

OffsetEffect::OffsetEffect(BaseLedPipelineStage *stage, float offset)
        : WrapperEffect(stage),
          offset(offset) {
    this->blendingMode = stage->blendingMode;
}


void OffsetEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    this->stage->calculate(startIndex + offset, tempData);
}

OffsetEffect::OffsetEffect(BaseLedPipelineStage *stage, const OffsetEffect::Config &config)
        : OffsetEffect(stage, config.offset) {}
