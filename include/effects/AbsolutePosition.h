#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct AbsolutePosition : WrapperEffect {
		struct Builder : WrapperEffect::Builder<AbsolutePosition> {

			BUILDER_FIELD_DEFAULT(float, position, 0);

			explicit Builder(float position = 0) : _position(position) {};

			AbsolutePosition* build() override {
				return new AbsolutePosition(_stage, _position);
			};
		};

		float position;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

	private:
		explicit AbsolutePosition(LedPipelineStage* stage, float position = 0);
	};
} // namespace ledpipelines::effects
