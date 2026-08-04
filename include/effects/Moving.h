#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Moving : public WrapperEffect, TimedEffect {
	private:
		// currentPosition is live runtime state (the interpolated position this frame), not an input. The inputs are
		// Value<T> recipes caching their own draw (see Value): plain values are constants, or pass a RandomRange<T> for
		// randomized endpoints/duration. calculate() reads startPosition.value / endPosition.value.
		float currentPosition;
		Value<float> startPosition;
		Value<float> endPosition;
		Value<SmoothingFunction> smoothingFunction;

	public:
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<Moving, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD_DEFAULT(float, startPosition, 0.0f);
			BUILDER_FIELD_DEFAULT(float, endPosition, TemporaryLedData::size);
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);

			explicit Builder(Value<unsigned long> runtimeMs) : TimedEffect::Builder<Builder>(std::move(runtimeMs)) {};

			Moving* build() override {
				return applyTiming(
					new Moving(buildInner(), _runtimeMs, _startPosition, _endPosition, _smoothingFunction));
			}
		};

	protected:
		Moving(LedPipelineStage* stage,
			   Value<unsigned long> runtimeMs,
			   Value<float> startPosition,
			   Value<float> endPosition,
			   Value<SmoothingFunction> smoothingFunction);
	};
} // namespace ledpipelines::effects
