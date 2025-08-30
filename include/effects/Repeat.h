#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
class Repeat : public WrapperEffect {
public:

    struct Config {
        RequiredField<float> repeatDistance;
        int numRepeats = 0;
    };

    int numRepeats = 1;
    float repeatDistance = 0;

    Repeat(BaseLedPipelineStage *stage, const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};

}