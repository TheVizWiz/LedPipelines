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

TimedEffect::TimedEffect(Value<unsigned long> runtimeMs)
	: startTimeMs(0), elapsedPercentage(0), runtimeMs(std::move(runtimeMs)) {
	// Sample the duration once for this lifecycle (a constant always yields the same value; a RandomRange draws one).
	// The result is cached in runtimeMs.value.
	this->runtimeMs.sample();
}


void TimedEffect::resetTimer() {
	this->elapsedPercentage = 0;
	// Re-roll the duration and lead-in delay for the next run. For constants these are no-ops; for a RandomRange they draw
	// fresh values - the "sample once per lifecycle, resample on reset" contract that the old RandomTimedEffect family
	// provided.
	this->runtimeMs.sample();
	this->delayMs.sample();
}
