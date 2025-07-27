
#include "effects/FlipEffect.h"


namespace ledpipelines::effects {
FlipEffect::FlipEffect(ledpipelines::BaseLedPipelineStage *stage, long min, long max)
        : min(std::min(min, max)),
          max(std::max(min, max)),
          WrapperEffect(stage) {}


void FlipEffect::calculate(float startIndex, ledpipelines::TemporaryLedData &tempData) {

    if (this->state == LedPipelineRunningState::DONE)
        return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state = LedPipelineRunningState::RUNNING;
    }

    stage->calculate(startIndex, tempData);

    for (int i = min; i < max / 2; i++) {
        auto leftOpacity = tempData.getOpacity(i);
        auto leftRGB = tempData.get(i);

        tempData.set(i, tempData.get(max - i), tempData.getOpacity(max - i));
        tempData.set(max - i, leftRGB, leftOpacity);
    }
    this->state = this->stage->state;
}

}
