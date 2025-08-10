#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
class OffsetEffect : public WrapperEffect {

public:

    struct Config {
        RequiredField<float> offset;
    };

    float offset;

public:
    OffsetEffect(BaseLedPipelineStage *stage, float offset);

    OffsetEffect(BaseLedPipelineStage *stage, const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};

}