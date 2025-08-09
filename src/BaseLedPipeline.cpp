
#include "BaseLedPipeline.h"

using namespace ledpipelines;

BaseLedPipelineStage::BaseLedPipelineStage(BlendingMode blendingMode)
        : blendingMode(blendingMode),
          state(LedPipelineRunningState::NOT_STARTED),
          lastUpdateTimeMicros(micros()) {}

BaseLedPipelineStage::~BaseLedPipelineStage() {
    delete nextStage;
}

void BaseLedPipelineStage::reset() {
    this->state = LedPipelineRunningState::NOT_STARTED;
}

void BaseLedPipelineStage::run() {
    if (this->state == LedPipelineRunningState::DONE) {
        return;
    }

    auto currentTimeMicros = micros();
    auto microsSinceLastUpdate = currentTimeMicros - lastUpdateTimeMicros;
    if (microsSinceLastUpdate > minMicrosBetweenUpdates) {
        lastUpdateTimeMicros = currentTimeMicros;
    } else {
        return;
    }
    FastLED.clear();
    TemporaryLedData data = TemporaryLedData();
    this->calculate(0, data);
    data.populateFastLed();
    FastLED.show();
}

LedPipeline::LedPipeline(BlendingMode mode) : BaseLedPipelineStage(mode) {}

LedPipeline::~LedPipeline() {
    delete firstStage;
}

LedPipeline *LedPipeline::addStage(BaseLedPipelineStage *stage) {
    if (this->firstStage == nullptr) {
        this->firstStage = stage;
        this->lastStage = stage;
    } else {
        this->lastStage->nextStage = stage;
        this->lastStage = stage;
    }
    return this;
}

void LedPipeline::reset() {
    BaseLedPipelineStage::reset();
    BaseLedPipelineStage *currentStage = firstStage;
    while (currentStage != nullptr) {
        currentStage->reset();
        currentStage = currentStage->nextStage;
    }
}

ParallelLedPipeline::ParallelLedPipeline(BlendingMode mode) : LedPipeline(mode) {}

void ParallelLedPipeline::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE)
        return;

    if (this->state == LedPipelineRunningState::NOT_STARTED)
        this->state = LedPipelineRunningState::RUNNING;

    BaseLedPipelineStage *currentStage = firstStage;

    if (currentStage == nullptr) {
        LPLogger::warn("no stages in the pipeline.");
    }

    LedPipelineRunningState anyArePlaying = LedPipelineRunningState::DONE;
    int currentStageNumber = 0;
    while (currentStage != nullptr) {
        TemporaryLedData currentStageData = TemporaryLedData();
        currentStage->calculate(startIndex, currentStageData);
//        LPLogger::log(String("blending stage ") + currentStageNumber + " with mode " + currentStage->blendingMode);
//        currentStageData.printData();
        tempData.merge(currentStageData, currentStage->blendingMode);
//        tempData.printData();
        anyArePlaying =
                currentStage->state != LedPipelineRunningState::DONE ? LedPipelineRunningState::RUNNING : anyArePlaying;
        currentStage = currentStage->nextStage;
        currentStageNumber++;
    }

    this->state = anyArePlaying;
}

SeriesLedPipeline::SeriesLedPipeline(BlendingMode mode) : LedPipeline(mode) {}

void SeriesLedPipeline::calculate(float startIndex, TemporaryLedData &tempData) {
    if (this->state == LedPipelineRunningState::DONE)
        return;

    // this is the first time we are state, so currentStage hasn't been set to the first stage yet.
    if (this->state == LedPipelineRunningState::NOT_STARTED) {
        LPLogger::log("starting series LED pipeline.");
        currentStage = firstStage;
        this->state = LedPipelineRunningState::RUNNING;
    }

    // there are no stages in the pipeline. return early.
    if (currentStage == nullptr) {
        LPLogger::warn("No stages in series pipeline. Exiting early.");
        this->state = LedPipelineRunningState::DONE;
        return;
    }

    // run the current stage. If the current stage is done state, we set the current stage to the next stage.
    currentStage->calculate(startIndex, tempData);
    if (currentStage->state == LedPipelineRunningState::DONE) {
        currentStage = currentStage->nextStage;
    }

    // if the current stage is null, we know that the pipeline is done state. We can set state to false.
    if (currentStage == nullptr) {
        this->state = LedPipelineRunningState::DONE;
        LPLogger::log("Series pipeline is done state.");
    }
}

void SeriesLedPipeline::reset() {
    LedPipeline::reset();
    this->currentStage = nullptr;
}


