#include "effects/OffsetEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

OffsetEffect::OffsetEffect(BaseLedPipelineStage *stage, int offset)
        : WrapperEffect(stage),
          offset(offset) {
    this->blendingMode = stage->blendingMode;
}


void OffsetEffect::calculate(int startIndex, TemporaryLedData &tempData) {
    this->stage->calculate(startIndex + offset, tempData);
}