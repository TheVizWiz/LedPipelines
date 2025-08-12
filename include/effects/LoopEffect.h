#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
class LoopEffect : public WrapperEffect {

public:
    struct Config {
        size_t numLoops = 0;
    };

private:
    size_t numLoops = 0;
    size_t currentNumLoops = 0;

public:

    explicit LoopEffect(BaseLedPipelineStage *stage, const Config &config = {.numLoops = 0});

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}