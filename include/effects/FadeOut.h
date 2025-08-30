#pragma once


#include "BaseEffect.h"
#include "FadeIn.h"


namespace ledpipelines::effects {
class FadeOut : public BaseLedPipelineStage, TimedEffect {

public:

    using Config = FadeIn::Config;

    SmoothingFunction smoothingFunction;

    FadeOut(const Config &config);

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