#include "effects/OpacityScale.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

OpacityScale::OpacityScale(LedPipelineStage* stage, uint8_t maxOpacity)
	: WrapperEffect(stage), maxOpacity(maxOpacity) {}

void OpacityScale::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	this->stage->calculate(startIndex, tempData);
	for (int i = 0; i < tempData.size; i++) {
		tempData.data[i].a = (tempData.data[i].a * maxOpacity) / 255;
	}

	this->state = this->stage->state;
}
