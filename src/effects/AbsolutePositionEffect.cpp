#include "effects/AbsolutePositionEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

AbsolutePositionEffect::AbsolutePositionEffect(BaseLedPipelineStage *stage, float position)
        : WrapperEffect(stage), position(position) {}

void AbsolutePositionEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    this->stage->calculate(position, tempData);
    this->state = this->stage->state;
}

AbsolutePositionEffect::AbsolutePositionEffect(BaseLedPipelineStage *stage,
                                               const AbsolutePositionEffect::Config &config)
        : AbsolutePositionEffect(stage, config.position) {}


