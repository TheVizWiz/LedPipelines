#include "effects/Shared.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Shared::Shared(std::shared_ptr<LedPipelineStage> stage, BlendingMode blendingMode) :
	LedPipelineStage(blendingMode), stage(std::move(stage)) {}

void Shared::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	this->stage->calculate(startIndex, tempData);

	this->state = this->stage->state;
}

void Shared::reset() {
	LedPipelineStage::reset();
	this->stage->reset();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Shared::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Shared");
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader());
	result += "}";
	return result;
}
#endif
