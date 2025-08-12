#pragma once


#include "BaseEffect.h"
#include "FadeInEffect.h"


namespace ledpipelines::effects {
class FadeOutEffect : public BaseLedPipelineStage, TimedEffect {

public:

    using Config = FadeInEffect::Config;

    SmoothingFunction smoothingFunction;

    FadeOutEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

class RandomFadeOutEffect : public BaseLedPipelineStage, RandomTimedEffect {

public:

    using Config = RandomFadeInEffect::Config;

    SmoothingFunction smoothingFunction;

    RandomFadeOutEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}