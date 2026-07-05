#include "effects/Repeat.h"

#include <cmath>

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

	if (numRepeats == 0) {
		// Infinite repeats. The tiled pattern is periodic with repeatDistance, so rendering the wrapped stage at
		// `startIndex` and tiling gives the SAME on-strip result as rendering it at `startIndex - m*repeatDistance`
		// for any integer m. We exploit that: render the base ONCE at a canonical offset reduced into [0,
		// repeatDistance) and nudged toward strip center, so the base render isn't clipped by the strip edges the way
		// it would be when startIndex pushes it off an end. Then we tile copies of that unclipped base both
		// directions. This is what stops the repeats from inheriting the original's edge-cropping: the copies are
		// shifts of a full render, not shifts of an already-truncated one.
		//
		// (Rendering once, rather than re-running the stage at each offset, also keeps stateful/timed children - Loop,
		// Series, Moving - advancing at the correct rate instead of many times per frame.)
		float canonical = fmodf(startIndex, repeatDistance);
		if (canonical < 0) canonical += repeatDistance; // fmodf keeps the sign of the dividend; force into [0, dist)
		// Center the canonical render within the strip: shift it by whole periods to sit as close to size/2 as
		// possible, maximizing the room the wrapped effect has on both sides before it clips.
		float center = TemporaryLedData::size / 2.0f;
		canonical += repeatDistance * roundf((center - canonical) / repeatDistance);

		TemporaryLedData stageData = TemporaryLedData();
		this->stage->calculate(canonical, stageData);
		this->state = this->stage->state;

		// Tile forward from the canonical render until a copy lights nothing, then backward likewise. Offsets are
		// relative to the canonical position; the pattern is identical to tiling from startIndex.
		float offset = 0;
		while (true) {
			TemporaryLedData copy = stageData.shift(offset);
			bool setsData = copy.anyAreModified;
			tempData.merge(copy, this->repeatBlendingMode);
			if (!setsData) break;
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
		// Finite repeats: absolute placement matters (N copies anchored at startIndex), so render at startIndex and
		// tile forward. Copies still inherit any edge-clipping of the base render; fixing that for finite mode is
		// deferred (see the infinite branch above for the unclipped-render approach).
		TemporaryLedData stageData = TemporaryLedData();
		this->stage->calculate(startIndex, stageData);
		this->state = this->stage->state;
		for (int i = 0; i < numRepeats; i++) {
			TemporaryLedData copy = stageData.shift(i * repeatDistance);
			tempData.merge(copy, this->repeatBlendingMode);
		}
	}
}
