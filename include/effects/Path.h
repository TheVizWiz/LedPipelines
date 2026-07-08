#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Path : WrapperEffect {
		using Segments = std::vector<std::pair<int, int>>;

		Segments segments;

		Path* addSegment(int start, int end);

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : WrapperEffect::Builder<Path, Builder> {
			BUILDER_FIELD_DEFAULT(Segments, segments, Segments());

			Builder& addSegment(int start, int end) {
				this->_segments.push_back(std::make_pair(start, end));
				return *this;
			}

			explicit Builder();

			Path* build() override {
				return new Path(buildInner(), std::move(_segments));
			}
		};

	protected:
		Path(LedPipelineStage* stage, const Segments& segments);

		Path(LedPipelineStage* stage, Segments&& segments);
	};
} // namespace ledpipelines::effects
