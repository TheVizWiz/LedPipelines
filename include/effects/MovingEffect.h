#pragma once


#include "BaseEffect.h"
#include "LedPipelineUtils.h"


namespace ledpipelines::effects {
class MovingEffect : public WrapperEffect {
public:
    struct Config {
        RequiredField<unsigned long> runtimeMs;
        float startPosition = 0;
        float endPosition = TemporaryLedData::size;
        SmoothingFunction smoothingFunction = SmoothingFunction::LINEAR;
    };

private:
    unsigned long runtimeMs;
    float currentPosition;
    float startPosition;
    float endPosition;
    float elapsedPercentage;
    unsigned long startTimeMs;
    SmoothingFunction smoothingFunction;

public:
    MovingEffect(BaseLedPipelineStage *stage, unsigned long runtimeMs, float startPosition = 0,
                 float endPosition = TemporaryLedData::size,
                 SmoothingFunction smoothingFunction = SmoothingFunction::LINEAR);

    MovingEffect(BaseLedPipelineStage *stage, const MovingEffect::Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}