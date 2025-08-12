#include "effects/BaseEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

WrapperEffect::WrapperEffect(BaseLedPipelineStage *stage) :
        BaseLedPipelineStage(stage->blendingMode),
        stage(stage) {}

WrapperEffect::~WrapperEffect() {
    delete stage;
}

void WrapperEffect::reset() {
    BaseLedPipelineStage::reset();
    stage->reset();
}

TimedEffect::TimedEffect(const TimedEffect::Config &config) :
        runtimeMs(config.runtimeMs) {}

void TimedEffect::reset() {
    this->elapsedPercentage = 0;
}

RandomTimedEffect::RandomTimedEffect(const RandomTimedEffect::Config &config)
        : TimedEffect({.runtimeMs = 0}),
          minRuntime(config.minRuntimeMs),
          maxRuntime(config.maxRuntimeMs),
          samplingFunction(config.samplingFunction) {}

void RandomTimedEffect::reset() {
    TimedEffect::reset();
}


void RandomTimedEffect::sampleRuntime() {
    this->runtimeMs = samplingFunction(minRuntime, maxRuntime);
}

