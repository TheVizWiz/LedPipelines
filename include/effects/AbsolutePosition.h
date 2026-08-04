#pragma once


#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct AbsolutePosition : WrapperEffect {
		struct Builder : WrapperEffect::Builder<AbsolutePosition, Builder> {

			BUILDER_FIELD_DEFAULT(float, position, 0);

			explicit Builder(Value<float> position = 0.0f) : _position(std::move(position)) {};

			AbsolutePosition* build() override {
				return new AbsolutePosition(buildInner(), _position);
			};
		};

		// The absolute position: a Value<float> caching its own draw (see Value). A plain float is a constant; a
		// RandomRange/RandomSet re-rolls on reset(). calculate() reads position.value.
		Value<float> position;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;


#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

	private:
		explicit AbsolutePosition(LedPipelineStage* stage, Value<float> position = 0.0f);
	};
} // namespace ledpipelines::effects
