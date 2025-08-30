#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {

struct Toggle : public WrapperEffect {

public:
    bool isActive;

    Toggle(BaseLedPipelineStage *stage);

    void deactivate();

    void activate();

    void toggle();

    void reset() override;

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};
}