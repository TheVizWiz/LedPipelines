#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
class Solid : public BaseLedPipelineStage {
public:

    struct Config {
        RequiredField<CRGB> color;
        uint8_t opacity = 0xFF;
    };



    CRGB color;
    uint8_t opacity;

    Solid(const Config &config);


    void calculate(float startIndex, TemporaryLedData &tempData) override;
};


class SolidSegment : public Solid {

public:
    struct Config {
        RequiredField<float> length;
        RequiredField<CRGB> color;
        uint8_t opacity = 0xFF;
    };

    float length;

    SolidSegment(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};



struct LengtheningSegment : public SolidSegment {

    struct Config {
        float startLength = 0;
        RequiredField<float> endLength;
        RequiredField<long> runtimeMs;
        RequiredField<CRGB> color;
        uint8_t opacity = 0xFF;
    };

    LengtheningSegment(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;
};

}