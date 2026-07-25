#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct AbsolutePosition : WrapperEffect {
		struct Builder : WrapperEffect::Builder<AbsolutePosition, Builder> {

			BUILDER_FIELD_DEFAULT(float, position, 0);

			explicit Builder(float position = 0) : _position(position) {};

			AbsolutePosition* build() override {
				return new AbsolutePosition(buildInner(), _position);
			};
		};

		float position;

		void calculate(float startIndex, TemporaryLedData& tempData) override;


#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

	private:
		explicit AbsolutePosition(LedPipelineStage* stage, float position = 0);
	};
} // namespace ledpipelines::effects
