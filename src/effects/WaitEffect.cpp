#include <utility>

#include "effects/WaitEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


WaitEffect::WaitEffect(float waitTimeSeconds) : TimedEffect(waitTimeSeconds) {}

void WaitEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state =  LedPipelineRunningState::RUNNING;
        this->startTimeMillis = millis();
    }

    unsigned long totalTimeWaited = millis() - this->startTimeMillis;

    if (totalTimeWaited / 1000.0 >= this->runtimeMs) {
        this->elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }

    this->elapsedPercentage = ((float) totalTimeWaited / 1000.0f) / this->runtimeMs; // convert ms to seconds
    this->state =  LedPipelineRunningState::RUNNING;
}


void WaitEffect::reset() {
    BaseLedPipelineStage::reset();
    TimedEffect::reset();
}


RandomWaitEffect::RandomWaitEffect(float maxWaitTime, const ledpipelines::SamplingFunction &function)
        : RandomWaitEffect(0,
                           maxWaitTime,
                           function) {}

RandomWaitEffect::RandomWaitEffect(float minWaitTime, float maxWaitTime, const SamplingFunction &function)
        : BaseLedPipelineStage(),
          RandomTimedEffect(minWaitTime, maxWaitTime, function) {}


void RandomWaitEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) {
        return;
    }

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state =  LedPipelineRunningState::RUNNING;
        this->startTimeMillis = millis();
        this->sampleRuntime();
        LPLogger::log(String("running random wait effect for ") + this->runtimeMs + " seconds");
    }

    unsigned long totalTimeWaited = millis() - this->startTimeMillis;

    if (totalTimeWaited / 1000.0 >= this->runtimeMs) {
        LPLogger::log("done running random wait effect.");
        this->elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }

    this->elapsedPercentage = ((float) totalTimeWaited / 1000.0f) / this->runtimeMs; // convert ms to seconds
    this->state =  LedPipelineRunningState::RUNNING;
}

void RandomWaitEffect::reset() {
    BaseLedPipelineStage::reset();
    RandomTimedEffect::reset();
}


