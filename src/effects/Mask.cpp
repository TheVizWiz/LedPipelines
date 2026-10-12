#include "effects/Mask.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Mask::Mask(LedPipelineStage* base, LedPipelineStage* mask, bool useMaskRuntime, BlendingMode blendingMode)
	: LedPipelineStage(blendingMode), base(base), mask(mask), useMaskRuntime(useMaskRuntime) {}

Mask::~Mask() {
	delete base;
	delete mask;
}


void Mask::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) {
		return;
	}

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		this->state = LedPipelineRunningState::RUNNING;
	}

	TemporaryLedData maskData = TemporaryLedData();
	TemporaryLedData baseData = TemporaryLedData();
	this->mask->calculate(startIndex, maskData);
	this->base->calculate(startIndex, baseData);

	baseData.merge(maskData, BlendingMode::MASK);
	tempData.merge(baseData, BlendingMode::NORMAL);

	if (useMaskRuntime) {
		if (this->mask->state == LedPipelineRunningState::DONE) {
			this->state = LedPipelineRunningState::DONE;
		}
	} else {
		if (this->base->state == LedPipelineRunningState::DONE) {
			this->state = LedPipelineRunningState::DONE;
		}
	}
}


void Mask::reset() {
	LedPipelineStage::reset();
	base->reset();
	mask->reset();
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Mask::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Mask");
	result += jsonRawField("base", base->toJson(includeState));
	result += jsonRawField("mask", mask->toJson(includeState));
	result += jsonField("useMaskRuntime", useMaskRuntime);
	if (includeState) result += jsonStateObject(jsonStateHeader());
	result += "}";
	return result;
}
#endif
