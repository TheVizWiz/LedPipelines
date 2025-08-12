#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
class OpacityGradientEffect : public WrapperEffect {
public:

    struct Config {
        float startIndex = 0;
        RequiredField<float> endIndex;
        SmoothingFunction smoothingFunction = SmoothingFunction::SMOOTH_LINEAR;
    };

    float startIndex;
    float endIndex;
    SmoothingFunction smoothingFunction;

    OpacityGradientEffect(BaseLedPipelineStage *stage, const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;


private:
    void calculateForwardGradient(float startIndex, TemporaryLedData &tempData);

    void calculateBackwardGradient(float startIndex, TemporaryLedData &tempData);
};

}