
#include "effects/ToggleEffect.h"

using namespace ledpipelines::effects;

ToggleEffect::ToggleEffect(ledpipelines::BaseLedPipelineStage *stage)
    : WrapperEffect(stage) {}

void ToggleEffect::reset() {
    isActive = true;
    WrapperEffect::reset();
}

void ToggleEffect::calculate(float startIndex, TemporaryLedData &tempData) {
    if (!this->isActive) return;

    this->stage->calculate(startIndex, tempData);
}

void ToggleEffect::deactivate() {
    this->isActive = false;
}

void ToggleEffect::activate() {
    this->isActive = true;
}

void ToggleEffect::toggle() {
    this->isActive = !this->isActive;
}