
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
public:
    struct Config {
        RequiredField<unsigned long> runtimeMs;
    };

    unsigned long startTimeMillis;
    float elapsedPercentage;
    unsigned long runtimeMs;

    TimedEffect(const Config &config);

    void reset();
};


class RandomTimedEffect : public TimedEffect {
public:

    struct Config {
        unsigned long minRuntimeMs;
        RequiredField<unsigned long> maxRuntimeMs;
        SamplingFunction samplingFunction = SamplingFunction::UNIFORM;
    };

    unsigned long minRuntime;
    unsigned long maxRuntime;
    SamplingFunction samplingFunction;

    RandomTimedEffect(const Config &config);


    void sampleRuntime();

    void reset();
};

}





