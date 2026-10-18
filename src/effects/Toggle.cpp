#include "effects/Toggle.h"

using namespace ledpipelines::effects;

Toggle::Toggle(ledpipelines::LedPipelineStage* stage, bool completeOnDeactivate, bool resetOnActivate)
	: WrapperEffect(stage), isActive(true), completeOnDeactivate(completeOnDeactivate), resetOnActivate(resetOnActivate) {}

void Toggle::reset() {
	isActive = true;
	WrapperEffect::reset();
}

void Toggle::calculate(float startIndex, TemporaryLedData& tempData) {
	if (!this->isActive) return;

	this->stage->calculate(startIndex, tempData);

	this->state = this->stage->state;
}

void Toggle::deactivate() {
	if (!this->isActive) return;
	this->isActive = false;
	if (this->completeOnDeactivate) this->state = LedPipelineRunningState::DONE;
}

void Toggle::activate() {
	if (this->isActive) return;
	this->isActive = true;
	if (this->resetOnActivate || this->completeOnDeactivate) {
		WrapperEffect::reset();
	} else {
		this->state = this->stage->state;
	}
}

void Toggle::toggle() {
	if (this->isActive) {
		deactivate();
	} else {
		activate();
	}
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Toggle::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Toggle");
	result += jsonField("completeOnDeactivate", completeOnDeactivate);
	result += jsonField("resetOnActivate", resetOnActivate);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("isActive", isActive));
	result += "}";
	return result;
}
#endif
