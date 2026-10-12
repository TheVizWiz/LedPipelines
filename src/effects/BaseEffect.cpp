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

void WrapperEffect::calculate(float startIndex, TemporaryLedData& tempData) {
	stage->calculate(startIndex, tempData);
	this->state = stage->state;
}

TimedEffect::TimedEffect(Value<unsigned long> runtimeMs)
	: startTimeMs(0), elapsedPercentage(0), runtimeMs(std::move(runtimeMs)) {
	this->runtimeMs.sample();
}


void TimedEffect::resetTimer() {
	this->elapsedPercentage = 0;
	this->runtimeMs.sample();
	this->delayMs.sample();
}
