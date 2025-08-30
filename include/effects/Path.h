#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {


struct Path : public WrapperEffect {

    std::vector<std::pair<int, int>> segments;

    Path(BaseLedPipelineStage *stage);

    Path *addSegment(int start, int end);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}