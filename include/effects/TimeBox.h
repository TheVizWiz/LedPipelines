#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
class TimeBox : public WrapperEffect, public TimedEffect {

public:

    using Config = TimedEffect::Config;

    TimeBox(BaseLedPipelineStage *stage, const Config &config);

    void reset() override;

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};

class RandomTimeBoxedEffect : public WrapperEffect, public RandomTimedEffect {

public:
    using Config = RandomTimedEffect::Config;

    RandomTimeBoxedEffect(
            BaseLedPipelineStage *stage,
            const Config &config
    );

    void reset() override;

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};


}