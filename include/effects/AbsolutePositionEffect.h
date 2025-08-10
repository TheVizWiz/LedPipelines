#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {

class AbsolutePositionEffect : public WrapperEffect {

public:

    struct Config {
        RequiredField<float> position;
    };

    float position;

    explicit AbsolutePositionEffect(BaseLedPipelineStage *stage, float position = 0);

    AbsolutePositionEffect(BaseLedPipelineStage *stage, const AbsolutePositionEffect::Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};
}