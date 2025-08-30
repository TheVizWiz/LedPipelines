#include "effects/Mask.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Mask::Mask(
        BaseLedPipelineStage *base,
        BaseLedPipelineStage *mask,
        bool useMaskRuntime
) : base(base),
    mask(mask),
    useMaskRuntime(useMaskRuntime) {
}


void Mask::calculate(float startIndex, TemporaryLedData &tempData) {

    if (this->state == LedPipelineRunningState::DONE) {
        return;
    }

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state =  LedPipelineRunningState::RUNNING;
    }

    TemporaryLedData maskData = TemporaryLedData();
    TemporaryLedData baseData = TemporaryLedData();
    this->mask->calculate(startIndex, maskData);
    this->base->calculate(startIndex, baseData);

    // merge the two layers.
    baseData.merge(maskData, BlendingMode::MASK);
    tempData.merge(baseData, this->base->blendingMode);

    // The effect finishes when the base is done, so we check if the base effect is done here.
    if (useMaskRuntime) {
        if (this->mask->state == LedPipelineRunningState::DONE) {
            this->state = LedPipelineRunningState::DONE;
        }
    } else {
        if (this->base->state == LedPipelineRunningState::DONE) {
            this->state = LedPipelineRunningState::DONE;
        }
    }
}


void Mask::reset() {
    BaseLedPipelineStage::reset();
    base->reset();
    mask->reset();
}
