#include "effects/Loop.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


Loop::Loop(LedPipelineStage* stage, size_t numLoops) : WrapperEffect(stage), numLoops(numLoops) {}

void Loop::reset() {
	WrapperEffect::reset();
	currentNumLoops = 0;
}

void Loop::calculate(float startIndex, TemporaryLedData& tempData) {
	if (state == LedPipelineRunningState::DONE) {
		return;
	}

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	stage->calculate(startIndex, tempData);

	if (stage->state != LedPipelineRunningState::DONE) {
		return;
	}

	currentNumLoops++;
	if (numLoops != 0 && currentNumLoops >= numLoops) {
		this->state = LedPipelineRunningState::DONE;
	} else {
		this->state = LedPipelineRunningState::RUNNING;
		this->stage->reset();
	}
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Loop::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Loop");
	result += jsonField("numLoops", (int)numLoops);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("currentNumLoops", (int)currentNumLoops));
	result += "}";
	return result;
}
#endif
