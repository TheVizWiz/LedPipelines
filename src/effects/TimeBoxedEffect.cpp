#include "effects/TimeBoxedEffect.h"
#include "LedPipelinesLogger.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

TimeBoxedEffect::TimeBoxedEffect(BaseLedPipelineStage *stage, float timeToRunSeconds)
        : WrapperEffect(stage), TimedEffect(timeToRunSeconds) {}


void TimeBoxedEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        LPLogger::log(String("starting time boxed effect. Running for ") + runtimeMs + " seconds");
        this->state =  LedPipelineRunningState::RUNNING;
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
        ledpipelines::BaseLedPipelineStage *stage,
        float maxRuntime,
        ledpipelines::SamplingFunction samplingFunction
) : RandomTimeBoxedEffect(stage, 0, maxRuntime, samplingFunction) {}

RandomTimeBoxedEffect::RandomTimeBoxedEffect(
        ledpipelines::BaseLedPipelineStage *stage,
        float minRuntime,
        float maxRuntime,
        ledpipelines::SamplingFunction samplingFunction
) : WrapperEffect(stage), RandomTimedEffect(minRuntime, maxRuntime, samplingFunction) {}

void RandomTimeBoxedEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state =  LedPipelineRunningState::RUNNING;
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






















