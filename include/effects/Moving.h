#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Moving : public WrapperEffect, TimedEffect {
		private:
			float currentPosition;
			float startPosition;
			float endPosition;
			SmoothingFunction smoothingFunction;

		public:
			void calculate(float startIndex, TemporaryLedData &tempData) override;

			void reset() override;

			struct Builder : WrapperEffect::Builder<Moving, Builder>, TimedEffect::Builder<Builder> {
				BUILDER_FIELD_DEFAULT(float, startPosition, 0.0f);
				BUILDER_FIELD_DEFAULT(float, endPosition, TemporaryLedData::size);
				BUILDER_FIELD_DEFAULT(
					SmoothingFunction, smoothingFunction,
					SmoothingFunction::LINEAR
				);

				explicit Builder(
					unsigned long runtimeMs
				) : TimedEffect::Builder<Builder>(runtimeMs) {};

				Moving *create() override {
					return new Moving(
						_stage,
						_runtimeMs,
						_startPosition,
						_endPosition,
						_smoothingFunction
					);
				}
			};

		protected:
			Moving(
				LedPipelineStage *stage,
				unsigned long runtimeMs,
				float startPosition,
				float endPosition,
				SmoothingFunction smoothingFunction
			);
	};
}
