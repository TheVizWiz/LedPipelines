
#include <utility>

#include "effects/MovingEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

MovingEffect::MovingEffect(
        BaseLedPipelineStage *stage,
        unsigned long runtimeMs,
        float startPosition,
        float endPosition,
        SmoothingFunction smoothingFunction
)
        : WrapperEffect(stage),
          currentPosition(0),
          startPosition(startPosition),
          endPosition(endPosition),
          runtimeMs(runtimeMs),
          elapsedPercentage(0),
          smoothingFunction(smoothingFunction) {}

MovingEffect::MovingEffect(BaseLedPipelineStage *stage, const MovingEffect::Config &config) :
        MovingEffect(
                stage,
                config.runtimeMs,
                config.startPosition,
                config.endPosition,
                config.smoothingFunction
        ) {
}

void MovingEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMs = millis();
        this->state = LedPipelineRunningState::RUNNING;
    }

    this->elapsedPercentage = (float) (millis() - this->startTimeMs) / (float) runtimeMs;

    this->currentPosition = smoothingFunction(elapsedPercentage, startPosition, endPosition);

    this->stage->calculate(startIndex + currentPosition, tempData);

    if (this->stage->state == LedPipelineRunningState::DONE || this->elapsedPercentage >= 1) {
        this->state = LedPipelineRunningState::DONE;
    }
}

void MovingEffect::reset() {
    WrapperEffect::reset();
    this->currentPosition = this->startPosition;
    this->elapsedPercentage = 0;
}
