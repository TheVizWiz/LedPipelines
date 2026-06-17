#include "effects/AbsolutePosition.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

AbsolutePosition::AbsolutePosition(LedPipelineStage *stage, float position)
	: WrapperEffect(stage), position(position) {}

void AbsolutePosition::calculate(float startIndex, TemporaryLedData &tempData) {
	this->stage->calculate(position, tempData);
	this->state = this->stage->state;
}


AbsolutePosition::Builder::Builder(float position) {
	this->position = position;
}

AbsolutePosition *AbsolutePosition::Builder::build() {
	return new AbsolutePosition(
		this->stage,
		this->position
	);
}
