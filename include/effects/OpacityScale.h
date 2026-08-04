#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct OpacityScale : public WrapperEffect {
		// The opacity ceiling: a Value<uint8_t> caching its own draw (see Value). A plain byte is a constant; a
		// RandomRange/RandomSet re-rolls on reset(). calculate() reads maxOpacity.value.
		Value<uint8_t> maxOpacity;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<OpacityScale, Builder> {
			BUILDER_FIELD_DEFAULT(uint8_t, maxOpacity, 0xFF);

			explicit Builder(Value<uint8_t> maxOpacity) : _maxOpacity(std::move(maxOpacity)) {}

			OpacityScale* build() override {
				return new OpacityScale(buildInner(), _maxOpacity);
			}
		};

	private:
		OpacityScale(LedPipelineStage* stage, Value<uint8_t> maxOpacity);
	};
} // namespace ledpipelines::effects
