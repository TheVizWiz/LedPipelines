#include "effects/ResetBlocker.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

ResetBlocker::ResetBlocker(LedPipelineStage* stage) : WrapperEffect(stage) {}

void ResetBlocker::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	this->stage->calculate(startIndex, tempData);

	this->state = this->stage->state;
}

void ResetBlocker::reset() {
	if (this->stage->state == LedPipelineRunningState::DONE) {
		WrapperEffect::reset();
	} else {
		LedPipelineStage::reset();
	}
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string ResetBlocker::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("ResetBlocker");
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader());
	result += "}";
	return result;
}
#endif
