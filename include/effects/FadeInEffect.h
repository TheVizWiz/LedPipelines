#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
class FadeInEffect : public BaseLedPipelineStage, TimedEffect {

public:

    struct Config {
        RequiredField<unsigned long> runtimeMs;
        SmoothingFunction smoothingFunction = SmoothingFunction::SMOOTH_LINEAR;
    };

    SmoothingFunction smoothingFunction;

    FadeInEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

class RandomFadeInEffect : public BaseLedPipelineStage, RandomTimedEffect {
public:

    struct Config {
        unsigned long minRuntimeMs = 0;
        RequiredField<unsigned long> maxRuntimeMs;
        SamplingFunction samplingFunction = SamplingFunction::UNIFORM;
        SmoothingFunction smoothingFunction = SmoothingFunction::SMOOTH_LINEAR;
    };

    SmoothingFunction smoothingFunction;

    RandomFadeInEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}