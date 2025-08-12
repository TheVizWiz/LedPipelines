#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
class SolidEffect : public BaseLedPipelineStage {
public:

    struct Config {
        RequiredField<CRGB> color;
        uint8_t opacity = 0xFF;
    };



    CRGB color;
    uint8_t opacity;

    SolidEffect(const Config &config);


    void calculate(float startIndex, TemporaryLedData &tempData) override;
};


class SolidSegmentEffect : public SolidEffect {

public:
    struct Config {
        RequiredField<float> length;
        RequiredField<CRGB> color;
        uint8_t opacity = 0xFF;
    };

    float length;

    SolidSegmentEffect(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};

}