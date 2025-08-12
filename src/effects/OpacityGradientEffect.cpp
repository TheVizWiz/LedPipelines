#include "effects/OpacityGradientEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


OpacityGradientEffect::OpacityGradientEffect(BaseLedPipelineStage *stage, const Config &config)
        : WrapperEffect(stage),
          startIndex(config.startIndex),
          endIndex(config.endIndex),
          smoothingFunction(config.smoothingFunction) {
}


void OpacityGradientEffect::calculateForwardGradient(float startIndex, TemporaryLedData &tempData) {
    int startIndexFloor = (int) startIndex;
    int endIndexFloor = (int) endIndex;

    /**
     *
     * How we light up a pixel:
     *
     * We calculate the opacity at the left bound of the pixel, and the opacity at the right bound of the pixel.
     * Then, the opacity of the pixel is multiplied by the average of the left and right bounds of the pixel.
     *
     * If the start and end are the same pixel, then set the left bound as the start and the right bound as the end,
     * and the pixel will be lit up by that range. Since we always light up from 0 to 100%, if the left and right
     * are on the same pixel, then we multiply the opacity by 50% (the average of left and right bounds).
     *
     * If the start and are not on the same pixel, we set:
     * - left bound of the first pixel as start (0%), and right bound as the right bound.
     * - left bound and right bound of every pixel between start and end as left and right bound.
     * - left bound of the last pixel as left bound, and right bound as the end value (100%).
     */
    if (startIndexFloor == endIndexFloor) {
        // both are on the same pixel, we can fade it partially. We fade it by 50% since the left and right bounds
        // are 0 and 1.
        float amountToFadePixel = 0.5;

        tempData.set(
                startIndexFloor,
                tempData.get(startIndexFloor),
                tempData.getOpacity(startIndexFloor) * amountToFadePixel
        );

    } else {

        // first pixel calculations
        float firstPixelLeftBound = 0;
        float firstPixelRightBound = smoothingFunction(
                floor(startIndex + 1),
                startIndex,
                endIndex,
                0,
                1
        );
        float firstPixelFadeAmount = (firstPixelLeftBound + firstPixelRightBound) / 2;

        // last pixel calculations
        float lastPixelLeftBound = smoothingFunction(
                floor(endIndex),
                startIndex,
                endIndex,
                0,
                1
        );
        float lastPixelRightBound = 1;
        float lastPixelFadeAmount = (lastPixelLeftBound + lastPixelRightBound) / 2;




        // set first pixel
        tempData.set(
                startIndexFloor,
                tempData.get(startIndexFloor),
                tempData.getOpacity(startIndexFloor) * firstPixelFadeAmount
        );


        // for every pixel in between, we calculate what the left and right bounds are, and average them.
        // to this, we use a clever trick: we know the left and right pixel values, and the average will be exactly halfway,
        // so we can just do left + 0.5. Then, the calculation will be much faster.
        for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
            float pixelAverageFadeAmount = smoothingFunction(
                    i + 0.5,
                    startIndex,
                    endIndex,
                    0,
                    1
            );

            tempData.set(
                    i,
                    tempData.get(i),
                    tempData.getOpacity(i) * pixelAverageFadeAmount
            );
        }

        // set last pixel
        tempData.set(
                endIndexFloor,
                tempData.get(endIndexFloor),
                tempData.getOpacity(endIndexFloor) * lastPixelFadeAmount
        );

    }

}


void OpacityGradientEffect::calculateBackwardGradient(float startIndex, TemporaryLedData &tempData) {


    // if the gradient is backwards, the left value is 1 and the right value is 0.
    // instead of doing complex math, we just flip how we are calculating start and end indices.
    int startIndexFloor = (int) startIndex;
    int endIndexFloor = (int) endIndex;

    /**
     *
     * How we light up a pixel:
     *
     * We calculate the opacity at the left bound of the pixel, and the opacity at the right bound of the pixel.
     * Then, the opacity of the pixel is multiplied by the average of the left and right bounds of the pixel.
     *
     * If the start and end are the same pixel, then set the left bound as the start and the right bound as the end,
     * and the pixel will be lit up by that range. Since we always light up from 0 to 100%, if the left and right
     * are on the same pixel, then we multiply the opacity by 50% (the average of left and right bounds).
     *
     * If the start and are not on the same pixel, we set:
     * - left bound of the first pixel as start (0%), and right bound as the right bound.
     * - left bound and right bound of every pixel between start and end as left and right bound.
     * - left bound of the last pixel as left bound, and right bound as the end value (100%).
     */
    if (startIndexFloor == endIndexFloor) {
        // both are on the same pixel, we can fade it partially. We fade it by 50% since the left and right bounds
        // are 0 and 1.
        float amountToFadePixel = 0.5;

        tempData.set(
                startIndexFloor,
                tempData.get(startIndexFloor),
                tempData.getOpacity(startIndexFloor) * amountToFadePixel
        );

    } else {

        // first pixel calculations
        float firstPixelLeftBound = 1;
        float firstPixelRightBound = smoothingFunction(
                floor(startIndex + 1),
                endIndex,
                startIndex,
                1,
                0
        );
        float firstPixelFadeAmount = (firstPixelLeftBound + firstPixelRightBound) / 2;

        // last pixel calculations
        float lastPixelLeftBound = smoothingFunction(
                floor(endIndex),
                endIndex,
                startIndex,
                1,
                0
        );
        float lastPixelRightBound = 0;
        float lastPixelFadeAmount = (lastPixelLeftBound + lastPixelRightBound) / 2;

        // set first pixel
        tempData.set(
                startIndexFloor,
                tempData.get(startIndexFloor),
                tempData.getOpacity(startIndexFloor) * firstPixelFadeAmount
        );


        // for every pixel in between, we calculate what the left and right bounds are, and average them.
        // to this, we use a clever trick: we know the left and right pixel values, and the average will be exactly halfway,
        // so we can just do left + 0.5. Then, the calculation will be much faster.
        for (int i = startIndexFloor + 1; i < endIndexFloor; i++) {
            float pixelAverageFadeAmount = smoothingFunction(
                    i + 0.5,
                    endIndex,
                    startIndex,
                    1,
                    0
            );

            tempData.set(
                    i,
                    tempData.get(i),
                    tempData.getOpacity(i) * pixelAverageFadeAmount
            );
        }

        // set last pixel
        tempData.set(
                endIndexFloor,
                tempData.get(endIndexFloor),
                tempData.getOpacity(endIndexFloor) * lastPixelFadeAmount
        );

    }
}

void OpacityGradientEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE)
        return;

    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        this->state = LedPipelineRunningState::RUNNING;
    }

    this->stage->calculate(startIndex, tempData);

    if (this->startIndex > this->endIndex) calculateBackwardGradient(startIndex + this->startIndex, tempData);
    else calculateForwardGradient(startIndex + this->startIndex, tempData);

    this->state = this->stage->state;
}
