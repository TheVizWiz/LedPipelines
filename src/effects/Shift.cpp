#include "effects/Shift.h"

using namespace ledpipelines;

using namespace ledpipelines::effects;

void Shift::calculate(const float startIndex, TemporaryLedData& tempData) {
	this->stage->calculate(startIndex + offset, tempData);
}

Shift::Shift(LedPipelineStage* stage, const float offset) : WrapperEffect(stage), offset(offset) {}
