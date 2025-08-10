
#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
class WrapperEffect : public BaseLedPipelineStage {
protected:
    BaseLedPipelineStage *stage;
public:
    explicit WrapperEffect(BaseLedPipelineStage *stage);

    ~WrapperEffect() override;

    void reset() override;
};


class TimedEffect {
protected:
    unsigned long startTimeMillis;
    float elapsedPercentage;
    unsigned long runtimeMs;

    TimedEffect(unsigned long runtimeMs);

    void reset();
};


class RandomTimedEffect : public TimedEffect {
protected:
    unsigned long minRuntime;
    unsigned long maxRuntime;
    SamplingFunction samplingFunction;

    RandomTimedEffect(
            unsigned long maxRuntime,
            SamplingFunction samplingFunction = SamplingFunction::UNIFORM
    );

    RandomTimedEffect(
            unsigned long minRuntime,
            unsigned long maxRuntime,
            SamplingFunction samplingFunction = SamplingFunction::UNIFORM
    );

    void sampleRuntime();

    void reset();
};

}





