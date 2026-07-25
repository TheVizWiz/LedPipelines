#include "effects/ResetBlocker.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

ResetBlocker::ResetBlocker(LedPipelineStage* stage) : WrapperEffect(stage) {}

void ResetBlocker::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	// Pure pass-through: the wrapper only exists to change reset() behavior, not the rendered output.
	this->stage->calculate(startIndex, tempData);

	this->state = this->stage->state;
}

void ResetBlocker::reset() {
	if (this->stage->state == LedPipelineRunningState::DONE) {
		// Inner has finished - a normal reset that propagates to the inner too.
		WrapperEffect::reset();
	} else {
		// Inner is still running - reset only our own state so we re-initialize next frame, but leave the inner
		// untouched so it can finish its current pass instead of being restarted.
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
