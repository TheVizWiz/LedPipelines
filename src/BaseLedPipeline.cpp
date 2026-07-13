#include "BaseLedPipeline.h"
#include "LedOutput.h"  // getOutput() - the registered render backend

using namespace ledpipelines;

LedPipelineStage::LedPipelineStage(BlendingMode blendingMode)
	: blendingMode(blendingMode), lastUpdateTimeMicros(micros()) {}

LedPipelineStage::~LedPipelineStage() = default;

void LedPipelineStage::reset() {
	this->state = LedPipelineRunningState::NOT_STARTED;
}

void LedPipelineStage::run() {
	if (this->state == LedPipelineRunningState::DONE) {
		return;
	}

	auto currentTimeMicros = micros();
	auto microsSinceLastUpdate = currentTimeMicros - lastUpdateTimeMicros;
	if (microsSinceLastUpdate > minMicrosBetweenUpdates) {
		lastUpdateTimeMicros = currentTimeMicros;
	} else {
		return;
	}
	// Render into the registered backend. setOutput() is required; if none is set, skip the frame (initialize() logs
	// the same condition) rather than dereferencing null.
	LedOutput* output = getOutput();
	if (output == nullptr) {
		return;
	}

	output->clear();
	TemporaryLedData data = TemporaryLedData();
	this->calculate(0, data);
	data.populate(*output);
	output->show();
}

LedPipeline::LedPipeline(BlendingMode mode) : LedPipelineStage(mode) {}

LedPipeline::~LedPipeline() = default;

LedPipeline* LedPipeline::addStage(std::unique_ptr<LedPipelineStage> stage) {
	this->stages.push_back(std::move(stage));
	return this;
}

LedPipeline* LedPipeline::addStage(LedPipelineStage* stage) {
	this->stages.push_back(std::unique_ptr<LedPipelineStage>(stage));
	return this;
}

void LedPipeline::reset() {
	LedPipelineStage::reset();

	for (auto& stage : stages) {
		// NOTE: stage->reset() (reset the pointed-to effect's state), NOT stage.reset() - the latter is
		// unique_ptr::reset(), which would delete the child stage and null the pointer, crashing the next calculate().
		stage->reset();
	}
}

ParallelLedPipeline::ParallelLedPipeline(BlendingMode mode) : LedPipeline(mode) {}

void ParallelLedPipeline::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;


	LedPipelineRunningState anyArePlaying = LedPipelineRunningState::DONE;

	for (auto& stage : stages) {
		TemporaryLedData currentData = TemporaryLedData();
		stage->calculate(startIndex, currentData);
		tempData.merge(currentData, stage->blendingMode);

		if (stage->state != LedPipelineRunningState::DONE) {
			anyArePlaying = LedPipelineRunningState::RUNNING;
		}
	}

	this->state = anyArePlaying;
}

SeriesLedPipeline::SeriesLedPipeline(const BlendingMode mode) : LedPipeline(mode) {}

void SeriesLedPipeline::calculate(const float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	// this is the first time we are state, so currentStage hasn't been set to the first stage yet.
	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		currentStage = 0;
		this->state = LedPipelineRunningState::RUNNING;
	}

	// Run the current stage; if it reports done, advance and run the next one in the SAME frame. Without this loop, the
	// frame on which a stage finishes would still show that finished stage's last output (a one-frame stall) before the
	// next stage got a chance to render. Looping also skips any stage that completes instantly on its first calculate.
	while (currentStage < stages.size()) {
		stages[currentStage]->calculate(startIndex, tempData);
		if (stages[currentStage]->state != LedPipelineRunningState::DONE) break;
		currentStage++;
		// Discard the finished stage's pixels so the next stage renders onto a clean buffer instead of layering over
		// leftover output. Only needed when we actually advance to another stage this frame.
		if (currentStage < stages.size()) tempData.clear();
	}

	if (currentStage == stages.size()) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void SeriesLedPipeline::reset() {
	LedPipeline::reset();
	this->currentStage = 0;
}
