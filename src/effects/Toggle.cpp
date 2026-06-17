#include "effects/Toggle.h"

using namespace ledpipelines::effects;

Toggle::Toggle(ledpipelines::LedPipelineStage *stage)
	: WrapperEffect(stage),
	  isActive(true) {
}

void Toggle::reset() {
	isActive = true;
	WrapperEffect::reset();
}

void Toggle::calculate(float startIndex, TemporaryLedData &tempData) {
	if (!this->isActive) return;

	this->stage->calculate(startIndex, tempData);

	this->state = this->stage->state;
}

void Toggle::deactivate() {
	this->isActive = false;
}

void Toggle::activate() {
	this->isActive = true;
}

void Toggle::toggle() {
	this->isActive = !this->isActive;
}