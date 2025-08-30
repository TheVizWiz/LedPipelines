
#include <utility>

#include "effects/Moving.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Moving::Moving(BaseLedPipelineStage *stage, const Moving::Config &config) :
        WrapperEffect(stage),
        currentPosition(0),
        startPosition(config.startPosition),
        endPosition(config.endPosition),
        runtimeMs(config.runtimeMs),
        elapsedPercentage(0),
        smoothingFunction(config.smoothingFunction) {}

void Moving::calculate(float startIndex, TemporaryLedData &tempData) {
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

void Moving::reset() {
    WrapperEffect::reset();
    this->currentPosition = this->startPosition;
    this->elapsedPercentage = 0;
}
