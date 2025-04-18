#pragma once


#include "BaseEffect.h"


class FadeOutEffect : public BaseLedPipelineStage, TimedEffect {

public:
    float fadeTime;
    LedPipelinesSmoothingType smoothingType;

    FadeOutEffect(
            float fadeTime,
            LedPipelinesSmoothingType smoothingType = SMOOTH_LINEAR);

    void calculate(int startIndex, TemporaryLedData &tempData) override;

    void reset() override;


};