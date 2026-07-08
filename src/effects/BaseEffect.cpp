#include "effects/BaseEffect.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


WrapperEffect::WrapperEffect(LedPipelineStage* stage) : stage(stage) {}

WrapperEffect::~WrapperEffect() {
	delete stage;
}

void WrapperEffect::reset() {
	LedPipelineStage::reset();
	stage->reset();
}

TimedEffect::TimedEffect(unsigned long runtimeMs) : startTimeMs(0), elapsedPercentage(0), runtimeMs(runtimeMs) {}


void TimedEffect::resetTimer() {
	this->elapsedPercentage = 0;
}

RandomTimedEffect::RandomTimedEffect(
	const unsigned long minRuntimeMs,
	const unsigned long maxRuntimeMs,
	SamplingFunction samplingFunction
)
	: TimedEffect(minRuntimeMs),
	  minRuntimeMs(minRuntimeMs),
	  maxRuntimeMs(maxRuntimeMs),
	  samplingFunction(std::move(samplingFunction)) {}

void RandomTimedEffect::resetTimer() {
	TimedEffect::resetTimer();
}

void RandomTimedEffect::sampleRuntime() {
	this->runtimeMs = samplingFunction(minRuntimeMs, maxRuntimeMs);
}
