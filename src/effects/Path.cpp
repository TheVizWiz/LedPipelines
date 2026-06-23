#include "effects/Path.h"


namespace ledpipelines::effects {
	Path::Path(LedPipelineStage* stage, const Segments& segments) : WrapperEffect(stage), segments(segments) {}

	Path::Path(LedPipelineStage* stage, Segments&& segments) : WrapperEffect(stage), segments(std::move(segments)) {}

	Path* Path::addSegment(int start, int end) {
		segments.emplace_back(start, end);
		return this;
	}

	void Path::calculate(float startIndex, TemporaryLedData& tempData) {
		if (this->state == LedPipelineRunningState::DONE) return;

		if (this->state == LedPipelineRunningState::NOT_STARTED) {
			this->state = LedPipelineRunningState::RUNNING;
		}

		TemporaryLedData stageData = TemporaryLedData();
		stage->calculate(startIndex, stageData);


		int currentIndex = 0;

		for (auto& segment : segments) {
			auto start = segment.first;
			auto end = segment.second;


			for (int i = start; (start < end ? i < end : i > end); (start < end ? i++ : i--)) {
				auto opacity = tempData.getOpacity(currentIndex);
				auto color = tempData.get(currentIndex);
				currentIndex++;

				if (opacity == 0) {
					// continue, don't rewrite this segment in case it's being written
					// in this path effect already.
					continue;
				}

				tempData.set(i, color, opacity);
			}
		}

		this->state = stage->state;
	}

	void Path::reset() { WrapperEffect::reset(); }
} // namespace ledpipelines::effects
