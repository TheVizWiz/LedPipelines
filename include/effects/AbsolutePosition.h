#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {

class AbsolutePosition : public WrapperEffect {

public:

    struct Config {
        RequiredField<float> position;
    };

    float position;

    explicit AbsolutePosition(BaseLedPipelineStage *stage, float position = 0);

    AbsolutePosition(BaseLedPipelineStage *stage, const AbsolutePosition::Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};
}