#include <utility>

#include "effects/FadeOutEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

FadeOutEffect::FadeOutEffect(
        float fadeTime,
        const SmoothingFunction &function
) :
        TimedEffect(fadeTime),
        smoothingFunction(function) {}


void FadeOutEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMillis = millis();
        this->state =  LedPipelineRunningState::RUNNING;
    }

    unsigned long currentTimeMillis = millis();

    float timeFadingSeconds = (currentTimeMillis - startTimeMillis) / 1000.0;

    // in this case, we have already finished fading, and can stop here.
    if (timeFadingSeconds >= timeToRunSeconds) {
        // when it's done, we still have to set it to done for the last frame.
        // so opacity is set to 0.
        for (int i = 0; i < TemporaryLedData::size; i++) {
            tempData.opacity[i] = 0;
        }
        elapsedPercentage = 1;
        this->state = LedPipelineRunningState::DONE;
        return;
    }
    elapsedPercentage = timeFadingSeconds / timeToRunSeconds;

    float opacityMultiplier = smoothingFunction(
            timeToRunSeconds,
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

RandomFadeOutEffect::RandomFadeOutEffect(
        float minFadeTime,
        float maxFadeTime,
        SmoothingFunction smoothingFunction,
        SamplingFunction samplingFunction
) : RandomTimedEffect(minFadeTime, maxFadeTime, samplingFunction),
    smoothingFunction(smoothingFunction) {}

void RandomFadeOutEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE) return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->startTimeMillis = millis();
        this->sampleRuntime();
        LPLogger::log(String("running random fade out effect for ") + this->timeToRunSeconds + " seconds");
        this->state =  LedPipelineRunningState::RUNNING;
    }

    unsigned long currentTimeMillis = millis();

    float timeFadingSeconds = (currentTimeMillis - startTimeMillis) / 1000.0;

    // in this case, we have already finished fading, and can stop here.
    if (timeFadingSeconds >= timeToRunSeconds) {
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

    elapsedPercentage = timeFadingSeconds / timeToRunSeconds;

    float currentOpacity = smoothingFunction(
            timeFadingSeconds,
            0,
            timeToRunSeconds,
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

