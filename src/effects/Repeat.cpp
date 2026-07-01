#include "effects/Repeat.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

Repeat::Repeat(LedPipelineStage* stage, int numRepeats, float repeatDistance, BlendingMode repeatBlendingMode) :
	WrapperEffect(stage), numRepeats(numRepeats), repeatDistance(repeatDistance),
	repeatBlendingMode(repeatBlendingMode) {}

void Repeat::calculate(float startIndex, TemporaryLedData& tempData) {
	if (this->state == LedPipelineRunningState::DONE) return;

	if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

	//    // only calculate the data once. We first calculate it at 0, and then shift it by how much
	//    TemporaryLedData stageData = TemporaryLedData();
	//    this->stage->calculate(0, stageData);
	//
	//    TemporaryLedData shiftedStageData = TemporaryLedData();
	//
	//    for (int i = 0; i < TemporaryLedData::ledCount; i++) {
	//        shiftedStageData.set(i + startIndex, stageData[i], stageData.opacity[i]);
	//    }
	//

	// Render the wrapped stage exactly once per frame, then tile that result by shifting copies of the rendered buffer.
	// Rendering once (rather than re-running the stage at each offset) keeps stateful/timed children - Loop, Series,
	// Moving - advancing at the correct rate instead of numRepeats times per frame.
	TemporaryLedData stageData = TemporaryLedData();
	this->stage->calculate(startIndex, stageData);
	this->state = this->stage->state;

	if (numRepeats == 0 && repeatDistance <= 0) {
		// In infinite-repeat mode a non-positive repeatDistance would never advance the offset in the loops below,
		// spinning forever (and, with pooled buffers, allocating without bound). Emit the single render instead.
		tempData.merge(stageData, this->repeatBlendingMode);
		return;
	}

	if (numRepeats == 0) {
		// Infinite repeats: tile the rendered buffer both forward and backward until a shifted copy lights nothing.
		float offset = 0;
		while (true) {
			TemporaryLedData copy = stageData.shift(offset);
			bool setsData = copy.anyAreModified;
			tempData.merge(copy, this->repeatBlendingMode);
			// Keep going while still on-strip in the negative direction even if this copy happened to light nothing.
			if (!setsData && offset >= 0) break;
			offset += repeatDistance;
		}
		offset = -repeatDistance;
		while (true) {
			TemporaryLedData copy = stageData.shift(offset);
			if (!copy.anyAreModified) break;
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
