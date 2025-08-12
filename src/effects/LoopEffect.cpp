#include "effects/LoopEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

LoopEffect::LoopEffect(BaseLedPipelineStage *stage, const LoopEffect::Config &config) :
        WrapperEffect(stage),
        numLoops(config.numLoops),
        currentNumLoops(0) {}

void LoopEffect::reset() {
    WrapperEffect::reset();
    currentNumLoops = 0;
}

void LoopEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (state == LedPipelineRunningState::DONE) {
        return;
    }

    if (this->state == LedPipelineRunningState::NOT_STARTED)
        this->state = LedPipelineRunningState::RUNNING;

    // run the current stage
    stage->calculate(startIndex, tempData);

    // check if the stage is done state. If still state, we return early.
    if (stage->state != LedPipelineRunningState::DONE) {
        return;
    }

    // if the current stage is done state, we increase the number of completed loops.
    // if the number of completed loops is high enough, we stop state the effect.
    // if the number of loops to run is 0, we run the effect infinitely.
    currentNumLoops++;
    if (numLoops != 0 && currentNumLoops >= numLoops) {
        LPLogger::log("Current stage done, and loops completed.");
        this->state = LedPipelineRunningState::DONE;
    } else {
        LPLogger::log(String("Current stage done, and looping again. Loops Completed: ")
                      + currentNumLoops + "/" + numLoops);
        this->state = LedPipelineRunningState::RUNNING;
        this->stage->reset();
        // since the stage might not calculate anything in done, we need to redo the stage.
//        this->stage->calculate(startIndex, tempData);
    }
}
