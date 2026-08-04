#pragma once

#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct OpacityGradient : WrapperEffect {
		// The gradient's endpoints: Value<float> recipes caching their own draw (see Value). Plain floats are constants;
		// a RandomRange/RandomSet re-rolls on reset(). calculate() reads startIndex.value / endIndex.value.
		Value<float> startIndex;
		Value<float> endIndex;
		SmoothingFunction smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<OpacityGradient, Builder> {
			BUILDER_FIELD_DEFAULT(float, startIndex, 0);
			BUILDER_FIELD(float, endIndex);
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::SMOOTH_LINEAR);

			explicit Builder(Value<float> endIndex) : _endIndex(std::move(endIndex)) {}

			explicit Builder(Value<float> startIndex, Value<float> endIndex)
				: _startIndex(std::move(startIndex)), _endIndex(std::move(endIndex)) {}

			OpacityGradient* build() override {
				return new OpacityGradient(buildInner(), _startIndex, _endIndex, _smoothingFunction.value);
			}
		};

	private:
		// endGradientIndex is the SAMPLED end value; the helpers take the offset-applied start position as their param.
		void calculateForwardGradient(float startPos, float endGradientIndex, TemporaryLedData& tempData) const;

		void calculateBackwardGradient(float startPos, float endGradientIndex, TemporaryLedData& tempData);

	protected:
		OpacityGradient(
			LedPipelineStage* stage,
			Value<float> startIndex,
			Value<float> endIndex,
			SmoothingFunction smoothingFunction
		);
	};
} // namespace ledpipelines::effects
