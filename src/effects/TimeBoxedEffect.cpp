#include "effects/TimeBoxedEffect.h"
#include "LedPipelinesLogger.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

TimeBoxedEffect::TimeBoxedEffect(BaseLedPipelineStage *stage, const Config &config)
        : WrapperEffect(stage), TimedEffect(config) {}


void TimeBoxedEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        LPLogger::log(String("starting time boxed effect. Running for ") + runtimeMs + " seconds");
        this->state = LedPipelineRunningState::RUNNING;
        startTimeMillis = millis();
    }

    unsigned long elapsedTime = millis() - startTimeMillis;


    this->elapsedPercentage = elapsedTime / 1000.0f / runtimeMs;
    this->stage->calculate(startIndex, tempData);
    // if the internal stage is done, we set it to done.
    if (this->stage->state == LedPipelineRunningState::DONE) {
        this->state = LedPipelineRunningState::DONE;
    }

    if (elapsedTime / 1000.0 >= runtimeMs) {
        LPLogger::log("done state time boxed effect.");
        this->elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }
}


void TimeBoxedEffect::reset() {
    WrapperEffect::reset();
    TimedEffect::reset();
}

RandomTimeBoxedEffect::RandomTimeBoxedEffect(
        BaseLedPipelineStage *stage,
        const Config &config)
        : WrapperEffect(stage), RandomTimedEffect(config) {}


void RandomTimeBoxedEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state = LedPipelineRunningState::RUNNING;
        this->sampleRuntime();
        LPLogger::log(String("running random time boxed effect for ") + this->runtimeMs + " seconds");
        startTimeMillis = millis();
    }

    unsigned long elapsedTime = millis() - startTimeMillis;


    this->elapsedPercentage = elapsedTime / 1000.0f / runtimeMs;
    this->stage->calculate(startIndex, tempData);
    // if the internal stage is done, we set it to done.
    if (this->stage->state == LedPipelineRunningState::DONE) {
        this->state = LedPipelineRunningState::DONE;
    }

    if (elapsedTime / 1000.0 >= runtimeMs) {
        LPLogger::log("done running random time boxed effect.");
        this->elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }
}


void RandomTimeBoxedEffect::reset() {
    WrapperEffect::reset();
    RandomTimedEffect::reset();
}






















