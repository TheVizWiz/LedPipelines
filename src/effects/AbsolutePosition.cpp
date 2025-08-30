#include "effects/AbsolutePosition.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

AbsolutePosition::AbsolutePosition(BaseLedPipelineStage *stage, float position)
        : WrapperEffect(stage), position(position) {}

void AbsolutePosition::calculate(float startIndex, TemporaryLedData &tempData) {
    this->stage->calculate(position, tempData);
    this->state = this->stage->state;
}

AbsolutePosition::AbsolutePosition(BaseLedPipelineStage *stage,
                                   const AbsolutePosition::Config &config)
        : AbsolutePosition(stage, config.position) {}


