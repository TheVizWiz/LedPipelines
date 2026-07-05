#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct OpacityGradient : WrapperEffect {
		float startIndex;
		float endIndex;
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<OpacityGradient, Builder> {
			BUILDER_FIELD_DEFAULT(float, startIndex, 0);
			BUILDER_FIELD(float, endIndex);
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			explicit Builder(const float endIndex) : _endIndex(endIndex) {}

			explicit Builder(const float startIndex, const float endIndex) : _startIndex(startIndex),
			                                                                 _endIndex(endIndex) {}

			OpacityGradient *create() override {
				return new OpacityGradient(
					buildInner(),
					_startIndex,
					_endIndex,
					_smoothingFunction
				);
			}
		};

		private:
			void calculateForwardGradient(float startIndex, TemporaryLedData &tempData) const;

			void calculateBackwardGradient(float startIndex, TemporaryLedData &tempData);

		protected:
			OpacityGradient(
				LedPipelineStage *stage,
				float startIndex,
				float endIndex,
				SmoothingFunction smoothingFunction
			);
	};
}
