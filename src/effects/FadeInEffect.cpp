#include "effects/FadeInEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

FadeInEffect::FadeInEffect(const FadeInEffect::Config &config)
        : TimedEffect({.runtimeMs = config.runtimeMs}),
          smoothingFunction(config.smoothingFunction) {}

void FadeInEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMillis = millis();
        this->state = LedPipelineRunningState::RUNNING;
    }

    unsigned long currentTimeMillis = millis();

    unsigned long timeFadingMs = (currentTimeMillis - startTimeMillis);

    // in this case, we have already finished fading, and can stop here.
    if (timeFadingMs >= runtimeMs) {
        elapsedPercentage = 1;
        // when it's done, we still have to set it to done for the last frame.
        // so opacity is set to 255.
        for (int i = 0; i < TemporaryLedData::size; i++) {
            tempData.opacity[i] = UINT8_MAX;
        }
        this->state = LedPipelineRunningState::DONE;
        return;
    } else {
        this->state = LedPipelineRunningState::RUNNING;
        elapsedPercentage = (float) timeFadingMs / (float) runtimeMs;
    }

    float opacityMultiplier = smoothingFunction(
            timeFadingMs,
            0,
            runtimeMs,
            0,
            UINT8_MAX
    );

    for (int i = 0; i < TemporaryLedData::size; i++) {
        tempData.opacity[i] = opacityMultiplier * 255;
    }
}

void FadeInEffect::reset() {
    BaseLedPipelineStage::reset();
    TimedEffect::reset();
}


RandomFadeInEffect::RandomFadeInEffect(const RandomFadeInEffect::Config &config)
        : RandomTimedEffect(
        {
                .minRuntimeMs = config.minRuntimeMs,
                .maxRuntimeMs =  config.maxRuntimeMs,
                .samplingFunction =  config.samplingFunction
        }),
          smoothingFunction(config.smoothingFunction) {}

void RandomFadeInEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMillis = millis();
        this->sampleRuntime();
        LPLogger::log(String("running random fade in effect for ") + this->runtimeMs + " seconds");
        this->state = LedPipelineRunningState::RUNNING;
    }

    unsigned long currentTimeMillis = millis();

    unsigned long timeFadingMs = (currentTimeMillis - startTimeMillis);

    // in this case, we have already finished fading, and can stop here.
    if (timeFadingMs >= runtimeMs) {
        LPLogger::log("done running random fade in effect.");
        elapsedPercentage = 1;
        // when it's done, we still have to set it to done for the last frame.
        // so opacity is set to 255.
        for (int i = 0; i < TemporaryLedData::size; i++) {
            tempData.opacity[i] = UINT8_MAX;
        }
        this->state = LedPipelineRunningState::DONE;
        return;
    } else {
        this->state = LedPipelineRunningState::RUNNING;
        elapsedPercentage = (float) timeFadingMs / (float) runtimeMs;
    }

    float currentOpacity = smoothingFunction(
            timeFadingMs,
            0,
            runtimeMs,
            0,
            UINT8_MAX
    );

    for (int i = 0; i < TemporaryLedData::size; i++) {
        tempData.opacity[i] = currentOpacity;
    }
}

void RandomFadeInEffect::reset() {
    BaseLedPipelineStage::reset();
    RandomTimedEffect::reset();
}
