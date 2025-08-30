#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
class Mask : public BaseLedPipelineStage {

public:
    BaseLedPipelineStage *mask;
    BaseLedPipelineStage *base;
    bool useMaskRuntime;

    Mask(
            BaseLedPipelineStage *base,
            BaseLedPipelineStage *mask,
            bool useMaskRuntime = false);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}
