#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {

class Flip : public WrapperEffect {
    long min;
    long max;


    Flip(BaseLedPipelineStage *stage, long min, long max);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

};


}
