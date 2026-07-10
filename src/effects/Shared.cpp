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

	// Delegate straight to the shared inner. Note the inner is a single instance that may be rendered from several
	// Shared wrappers this frame; we deliberately do not copy or guard its state (see the class comment - Shared is
	// meant for stateless sub-effects).
	this->stage->calculate(startIndex, tempData);

	this->state = this->stage->state;
}

void Shared::reset() {
	LedPipelineStage::reset();
	// Reset the shared inner too. If multiple Shared wrappers reference the same inner, each reset() re-resets it -
	// harmless for the stateless sources Shared is intended for.
	this->stage->reset();
}
