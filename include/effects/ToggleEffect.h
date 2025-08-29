#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {

struct ToggleEffect : public WrapperEffect {

public:
    bool isActive;

    ToggleEffect(BaseLedPipelineStage *stage);

    void deactivate();

    void activate();

    void toggle();

    void reset() override;

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};
}