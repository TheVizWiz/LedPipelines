#pragma once


#include "BaseEffect.h"

class FadeInEffect : public BaseLedPipelineStage, TimedEffect {

public:
    float fadeTime;
    LedPipelinesSmoothingType smoothingType;

    FadeInEffect(
            float fadeTime,
            LedPipelinesSmoothingType smoothingType = SMOOTH_LINEAR);

    void calculate(int startIndex, TemporaryLedData &tempData) override;

    void reset() override;


};