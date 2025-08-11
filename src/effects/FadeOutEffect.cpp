#include <utility>

#include "effects/FadeOutEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

FadeOutEffect::FadeOutEffect(const Config &config)
        : TimedEffect(config.fadeInTimeMs),
          smoothingFunction(config.smoothingFunction) {}

void FadeOutEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMillis = millis();
        this->state = LedPipelineRunningState::RUNNING;
    }

    unsigned long currentTimeMillis = millis();

    float timeFadingSeconds = (currentTimeMillis - startTimeMillis) / 1000.0;

    // in this case, we have already finished fading, and can stop here.
    if (timeFadingSeconds >= runtimeMs) {
        // when it's done, we still have to set it to done for the last frame.
        // so opacity is set to 0.
        for (int i = 0; i < TemporaryLedData::size; i++) {
            tempData.opacity[i] = 0;
        }
        elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }
    elapsedPercentage = timeFadingSeconds / runtimeMs;

    float opacityMultiplier = smoothingFunction(
            runtimeMs,
            0,
            timeFadingSeconds,
            UINT8_MAX,
            0
    );

    for (int i = 0; i < TemporaryLedData::size; i++) {
        tempData.opacity[i] = (tempData.opacity[i] * opacityMultiplier) / 255;
    }
}

void FadeOutEffect::reset() {
    BaseLedPipelineStage::reset();
    TimedEffect::reset();
}

RandomFadeOutEffect::RandomFadeOutEffect(const RandomFadeOutEffect::Config &config)
        : RandomTimedEffect(
        config.minFadeTimeMs,
        config.maxFadeTimeMs,
        config.samplingFunction
), smoothingFunction(config.smoothingFunction) {}


void RandomFadeOutEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMillis = millis();
        this->sampleRuntime();
        LPLogger::log(String("running random fade out effect for ") + this->runtimeMs + " seconds");
        this->state = LedPipelineRunningState::RUNNING;
    }

    unsigned long currentTimeMillis = millis();

    float timeFadingMs = (currentTimeMillis - startTimeMillis);

    // in this case, we have already finished fading, and can stop here.
    if (timeFadingMs >= runtimeMs) {
        LPLogger::log("done running random time boxed effect.");

        // when it's done, we still have to set it to done for the last frame.
        // so opacity is set to 0.
        for (int i = 0; i < TemporaryLedData::size; i++) {
            tempData.opacity[i] = 0;
        }
        elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }

    elapsedPercentage = (float) timeFadingMs / (float) runtimeMs;

    float currentOpacity = smoothingFunction(
            timeFadingMs,
            0,
            runtimeMs,
            UINT8_MAX,
            0
    );

    for (int i = 0; i < TemporaryLedData::size; i++) {
        tempData.opacity[i] = currentOpacity;
    }
}

void RandomFadeOutEffect::reset() {
    BaseLedPipelineStage::reset();
    RandomTimedEffect::reset();
}

