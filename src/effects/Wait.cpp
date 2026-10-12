#include <utility>

#include "effects/Wait.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;


Wait::Wait(Value<unsigned long> runtimeMs, BlendingMode blendingMode)
	: LedPipelineStage(blendingMode), TimedEffect(std::move(runtimeMs)) {}


void Wait::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
		this->startTimeMs = millis();
	}

	unsigned long totalTimeWaited = elapsedMs();

	if (totalTimeWaited >= this->runtimeMs.get()) {
		this->elapsedPercentage = 1;
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	this->elapsedPercentage = (float)totalTimeWaited / (float)this->runtimeMs.get();
	this->state = LedPipelineRunningState::RUNNING;
}

void Wait::reset() {
	LedPipelineStage::reset();
	TimedEffect::resetTimer();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Wait::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Wait");
	result += jsonRawField("runtimeMs", runtimeMs.describeValue());
	result += jsonRawField("delayMs", delayMs.describeValue());
	result += jsonField("terminateOnComplete", terminateOnComplete);
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonTimedEffect());
	result += "}";
	return result;
}
#endif
