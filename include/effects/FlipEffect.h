#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {

class FlipEffect : public WrapperEffect {
    long min;
    long max;


    FlipEffect(BaseLedPipelineStage *stage, long min, long max);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

};


}
