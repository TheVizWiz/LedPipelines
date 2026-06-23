#include "BaseLedPipeline.h"

using namespace ledpipelines;

LedPipelineStage::LedPipelineStage(BlendingMode blendingMode) : lastUpdateTimeMicros(micros()) {}

LedPipelineStage::~LedPipelineStage() = default;

void LedPipelineStage::reset() { this->state = LedPipelineRunningState::NOT_STARTED; }

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
	FastLED.clear();
	TemporaryLedData data = TemporaryLedData();
	this->calculate(0, data);
	data.populateFastLed();
	FastLED.show();
}

LedPipeline::LedPipeline(BlendingMode mode) : LedPipelineStage(mode) {}

LedPipeline* LedPipeline::addStage(std::unique_ptr<LedPipelineStage> stage) {}

LedPipeline* LedPipeline::addStage(LedPipelineStage* stage) {
	this->stages.push_back(std::unique_ptr<LedPipelineStage>(stage));
	return this;
}

void LedPipeline::reset() {
	LedPipelineStage::reset();

	for (auto& stage : stages) {
		stage.reset();
	}
}

ParallelLedPipeline::ParallelLedPipeline(BlendingMode mode) : LedPipeline(mode) {}

void ParallelLedPipeline::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;


	LedPipelineRunningState anyArePlaying = LedPipelineRunningState::DONE;

	for (auto& stage : stages) {
		stage->calculate(startIndex, tempData);

		TemporaryLedData currentData = TemporaryLedData();
		stage->calculate(startIndex, currentData);
		tempData.merge(currentData, this->blendingMode);

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

	// finished with all stages. Can set to done and return early.
	if (currentStage == stages.size()) {
		this->state = LedPipelineRunningState::DONE;
		return;
	}

	// run the current stage. If the current stage is done state, we set the current stage to the next stage.
	stages[currentStage]->calculate(startIndex, tempData);
	if (stages[currentStage]->state == LedPipelineRunningState::DONE) {
		currentStage++;
	}

	// if the current stage is null, we know that the pipeline is done state. We can set state to false.
	if (currentStage == stages.size()) {
		this->state = LedPipelineRunningState::DONE;
	}
}

void SeriesLedPipeline::reset() {
	LedPipeline::reset();
	this->currentStage = 0;
}
