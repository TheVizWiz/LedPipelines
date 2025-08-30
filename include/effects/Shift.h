#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
class Shift : public WrapperEffect {

public:

    struct Config {
        RequiredField<float> offset;
    };

    float offset;

public:
    Shift(BaseLedPipelineStage *stage, float offset);

    Shift(BaseLedPipelineStage *stage, const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};

}