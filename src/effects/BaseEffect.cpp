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


TimedEffect::TimedEffect(unsigned long runtimeMs) : runtimeMs(runtimeMs) {}

void TimedEffect::reset() {
    this->elapsedPercentage = 0;
}

RandomTimedEffect::RandomTimedEffect(unsigned long maxRuntime, SamplingFunction samplingFunction) :
        RandomTimedEffect(0, maxRuntime, samplingFunction) {}


void RandomTimedEffect::reset() {
    TimedEffect::reset();
}

void RandomTimedEffect::sampleRuntime() {
    this->runtimeMs = samplingFunction(minRuntime, maxRuntime);
}

RandomTimedEffect::RandomTimedEffect(unsigned long minRuntime, unsigned long maxRuntime, SamplingFunction samplingFunction)
        : TimedEffect(0),
          minRuntime(minRuntime),
          maxRuntime(maxRuntime),
          samplingFunction(samplingFunction) {

}

