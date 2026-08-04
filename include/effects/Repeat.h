#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Repeat : WrapperEffect {
		int numRepeats;
		// The spacing between repeated copies: a Value<float> caching its own draw (see Value). A plain float is a
		// constant; a RandomRange/RandomSet re-rolls on reset(). calculate() reads repeatDistance.value.
		Value<float> repeatDistance;
		// How the repeated copies stack onto each other within this effect. Distinct from the inherited blendingMode,
		// which controls how the whole Repeat result blends into its parent (a wrapper inherits that from its child).
		BlendingMode repeatBlendingMode;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<Repeat, Builder> {
			BUILDER_FIELD_DEFAULT(int, numRepeats, 0);
			BUILDER_FIELD(float, repeatDistance);
			BUILDER_FIELD_DEFAULT(BlendingMode, repeatBlendingMode, BlendingMode::NORMAL);

			Builder(Value<float> repeatDistance) : _repeatDistance(std::move(repeatDistance)) {};

			Repeat* build() override {
				return new Repeat(buildInner(), _numRepeats.value, _repeatDistance, _repeatBlendingMode.value);
			}
		};

	protected:
		Repeat(LedPipelineStage* stage, int numRepeats, Value<float> repeatDistance, BlendingMode repeatBlendingMode);
	};
} // namespace ledpipelines::effects
