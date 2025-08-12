#pragma once


#include "BaseEffect.h"
#include "LedPipelineUtils.h"

namespace ledpipelines::effects {
class WaitEffect : public BaseLedPipelineStage, TimedEffect {
public:

    using Config = TimedEffect::Config;

    WaitEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;

};


class RandomWaitEffect : public BaseLedPipelineStage, RandomTimedEffect {
public:

    using Config = RandomTimedEffect::Config;

    RandomWaitEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};
}