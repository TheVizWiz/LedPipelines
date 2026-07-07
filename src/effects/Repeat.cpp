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

	if (numRepeats == 0 && repeatDistance <= 0) {
		// In infinite-repeat mode a non-positive repeatDistance would never advance the offset in the loops below,
		// spinning forever (and, with pooled buffers, allocating without bound). Emit a single render instead.
		TemporaryLedData stageData = TemporaryLedData();
		this->stage->calculate(startIndex, stageData);
		this->state = this->stage->state;
		tempData.merge(stageData, this->repeatBlendingMode);
		return;
	}

	// Render the wrapped stage exactly once per frame, then tile that result by shifting copies. Rendering once (rather
	// than re-running the stage at each offset) keeps stateful/timed children - Loop, Series, Moving - advancing at the
	// correct rate instead of many times per frame. The base is rendered at the true startIndex; because
	// TemporaryLedData now has off-strip padding on both sides, a segment pushed partly past an edge keeps its
	// off-strip pixels intact in the padding, so the shifted copies below are faithful full copies rather than shifts
	// of an already-clipped render.
	TemporaryLedData stageData = TemporaryLedData();
	this->stage->calculate(startIndex, stageData);
	this->state = this->stage->state;

	if (numRepeats == 0) {
		// Infinite repeats: tile the rendered buffer forward and backward, stopping in each direction once a shifted
		// copy no longer lights any VISIBLE pixel. hasVisiblePixels() (not anyAreModified) is the right stop condition
		// because a copy can still light the off-strip padding after it has scrolled out of the visible window.
		float offset = 0;
		while (true) {
			TemporaryLedData copy = stageData.shift(offset);
			bool visible = copy.hasVisiblePixels();
			tempData.merge(copy, this->repeatBlendingMode);
			// Keep going while still visible; also take one step past the last visible copy on the very first
			// iteration so a base that starts off-screen (fully in the negative padding) doesn't stop us immediately.
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
		// Finite repeats: absolute placement matters (N copies anchored at startIndex), so tile forward exactly
		// numRepeats times. Copies now also benefit from the padded render (no edge-clipping of the base).
		for (int i = 0; i < numRepeats; i++) {
			TemporaryLedData copy = stageData.shift(i * repeatDistance);
			tempData.merge(copy, this->repeatBlendingMode);
		}
	}
}
