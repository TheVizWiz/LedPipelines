#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
class FadeOutEffect : public BaseLedPipelineStage, TimedEffect {

public:
    struct Config {
        RequiredField<unsigned long> fadeInTimeMs;
        SmoothingFunction smoothingFunction = SmoothingFunction::SMOOTH_LINEAR;
    };
    SmoothingFunction smoothingFunction;

    FadeOutEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

class RandomFadeOutEffect : public BaseLedPipelineStage, RandomTimedEffect {

public:
    struct Config {
        unsigned long minFadeTimeMs = 0;
        RequiredField<unsigned long> maxFadeTimeMs;
        SmoothingFunction smoothingFunction = SmoothingFunction::SMOOTH_LINEAR;
        SamplingFunction samplingFunction = SamplingFunction::UNIFORM;
    };

    SmoothingFunction smoothingFunction;

    RandomFadeOutEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}