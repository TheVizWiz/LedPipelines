#include "effects/Repeat.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Repeat::Repeat(LedPipelineStage* stage, int numRepeats, Value<float> repeatDistance, BlendingMode repeatBlendingMode)
	: WrapperEffect(stage),
	  numRepeats(numRepeats),
	  repeatDistance(std::move(repeatDistance)),
	  repeatBlendingMode(repeatBlendingMode) {
	this->repeatDistance.sample();
}

void Repeat::reset() {
	WrapperEffect::reset();
	this->repeatDistance.sample();
}

void Repeat::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	const float repeatDistance = this->repeatDistance.get();

	if (numRepeats == 0 && repeatDistance <= 0) {
		// In infinite-repeat mode a non-positive repeatDistance would never advance the offset in the loops below,
		// spinning forever (and, with pooled buffers, allocating without bound). Emit a single render instead.
		TemporaryLedData stageData = TemporaryLedData();
		this->stage->calculate(startIndex, stageData);
		this->state = this->stage->state;
		tempData.merge(stageData, this->repeatBlendingMode);
		return;
	}

	// Render once per frame, then tile shifted copies, so stateful/timed children (Loop, Series, Moving) advance at the
	// correct rate. The off-strip padding keeps pixels pushed past an edge, so copies are not shifts of a clipped render.
	TemporaryLedData stageData = TemporaryLedData();
	this->stage->calculate(startIndex, stageData);
	this->state = this->stage->state;

	if (numRepeats == 0) {
		// hasVisiblePixels() (not anyAreModified) is the stop condition because a copy can still light the off-strip
		// padding after it has scrolled out of the visible window.
		float offset = 0;
		while (true) {
			TemporaryLedData copy = stageData.shift(offset);
			bool visible = copy.hasVisiblePixels();
			tempData.merge(copy, this->repeatBlendingMode);
			// Never stop on the first iteration, so a base that starts fully in the negative padding still tiles forward.
			if (!visible && offset > 0) break;
			offset += repeatDistance;
		}
		offset = -repeatDistance;
		while (true) {
			TemporaryLedData copy = stageData.shift(offset);
			if (!copy.hasVisiblePixels()) break;
			tempData.merge(copy, this->repeatBlendingMode);
			offset -= repeatDistance;
		}
	} else {
		for (int i = 0; i < numRepeats; i++) {
			TemporaryLedData copy = stageData.shift(i * repeatDistance);
			tempData.merge(copy, this->repeatBlendingMode);
		}
	}
}


#ifdef LEDPIPELINES_SERIALIZATION
std::string Repeat::toJson(bool includeState) {
	std::string result = "{";
	result += jsonHeader("Repeat");
	result += jsonField("numRepeats", (int)numRepeats);
	result += jsonRawField("repeatDistance", repeatDistance.describeValue());
	result += jsonField("repeatBlendingMode", repeatBlendingMode);
	result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
	if (includeState) {
		result += jsonStateObject(jsonStateHeader() + jsonField("sampledRepeatDistance", repeatDistance.value));
	}
	result += "}";
	return result;
}
#endif
