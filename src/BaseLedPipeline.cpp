#include "BaseLedPipeline.h"
#include "LedOutput.h"

using namespace ledpipelines;

#ifdef LEDPIPELINES_SERIALIZATION
// Fill a 9-char id + NUL.
static void generateRandomId(char* id) {
	static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
								"abcdefghijklmnopqrstuvwxyz"
								"0123456789";

	for (int i = 0; i < 9; i++) {
		id[i] = chars[rand() % (sizeof(chars) - 1)];
	}
	id[9] = '\0';
}
#endif

LedPipelineStage::LedPipelineStage(BlendingMode blendingMode)
	: blendingMode(blendingMode), lastUpdateTimeMicros(micros()) {
#ifdef LEDPIPELINES_SERIALIZATION
	generateRandomId(id);
#endif
}

LedPipelineStage::~LedPipelineStage() {
	for (auto& registry : handleRegistries) registry->remove(this);
}

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
		// stage->reset(), NOT stage.reset() - the latter is unique_ptr::reset() and would delete the child.
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

	if (this->state == LedPipelineRunningState::NOT_STARTED) {
		currentStage = 0;
		this->state = LedPipelineRunningState::RUNNING;
	}

	// Advance through finished stages within the SAME frame, so a finishing stage doesn't cause a one-frame stall.
	while (currentStage < stages.size()) {
		stages[currentStage]->calculate(startIndex, tempData);
		if (stages[currentStage]->state != LedPipelineRunningState::DONE) break;
		currentStage++;
		// Discard the finished stage's pixels so the next stage doesn't layer over them.
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

#ifdef LEDPIPELINES_SERIALIZATION
std::string ParallelLedPipeline::toJson(bool includeState) {
	std::string children = "[";
	for (size_t i = 0; i < stages.size(); i++) {
		if (i > 0) children += ",";
		children += stages[i]->toJson(includeState);
	}
	children += "]";

	std::string result = "{";
	result += jsonHeader("ParallelLedPipeline");
	result += jsonRawField("children", children);
	if (includeState) result += jsonStateObject(jsonStateHeader());
	result += "}";
	return result;
}

std::string SeriesLedPipeline::toJson(bool includeState) {
	std::string children = "[";
	for (size_t i = 0; i < stages.size(); i++) {
		if (i > 0) children += ",";
		children += stages[i]->toJson(includeState);
	}
	children += "]";

	std::string result = "{";
	result += jsonHeader("SeriesLedPipeline");
	result += jsonRawField("children", children);
	if (includeState) result += jsonStateObject(jsonStateHeader() + jsonField("currentStage", currentStage));
	result += "}";
	return result;
}
#endif
