#include "effects/Solid.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Solid::Solid(const Solid::Config &config)
        : BaseLedPipelineStage(BlendingMode::NORMAL),
          color(config.color),
          opacity(config.opacity) {
    this->state = LedPipelineRunningState::RUNNING;
}

void Solid::calculate(float startIndex, TemporaryLedData &tempData) {
    for (int i = startIndex; i < TemporaryLedData::size; i++) {
        tempData.set(i, color, opacity);
    }
}

SolidSegment::SolidSegment(const SolidSegment::Config &config)
        : Solid({.color = config.color, .opacity = config.opacity}),
          length(config.length) {}

void SolidSegment::calculate(float startIndex, TemporaryLedData &tempData) {


    float endIndex = startIndex + length;

    int startIndexFloor = (int) startIndex;
    int endIndexFloor = (int) endIndex;

    /**
     * Two cases:
     *
     * if the start and end of the segment are in the same pixel, then we need to only light up that pixel, and only
     * partially based on how large the pixel value is.
     *
     * if the start is on a different pixel than the end, we can light up the first pixel partially, then light up
     * all pixels between start and end completely, and then light up the last pixel partially as well.
     */
    if (startIndexFloor == endIndexFloor) {
        // both are on the same pixel, we can light it up partially.
        float amountToLightUp = length;
        tempData.set(startIndexFloor, color * amountToLightUp, opacity);

    } else {


        float amountToLightUpFirstPixel = (1 - (startIndex - startIndexFloor));
        float amountToLightUpLastPixel = (endIndex - endIndexFloor);

        if (amountToLightUpFirstPixel != 0) {
            tempData.set(startIndexFloor, color, opacity * amountToLightUpFirstPixel);
        }

        for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
            tempData.set(i, color, opacity);
        }

        if (amountToLightUpLastPixel != 0) {
            tempData.set(endIndexFloor, color, opacity * amountToLightUpLastPixel);
        }


//        LPLogger::log(String("lit up first pixel ") + amountToLightUpFirstPixel + " and last pixel " +
//                      amountToLightUpLastPixel);


    }
}
