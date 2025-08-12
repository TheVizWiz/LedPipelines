#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {


struct PathEffect : WrapperEffect {

    std::vector<std::pair<int, int>> segments;

    PathEffect(BaseLedPipelineStage *stage);

    PathEffect *addSegment(int start, int end);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}