#pragma once


#include "BaseEffect.h"
#include "Value.h"

namespace ledpipelines::effects {
	// Shifts its inner effect along the strip by an offset, sampled once per lifecycle and re-rolled on reset().
	// useWholePixels snaps the effective offset to an integer pixel so the shifted copy lands exactly on LEDs (no
	// fractional-coverage feathering).
	struct Shift : WrapperEffect {
		Value<float> offset;
		bool useWholePixels;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<Shift, Builder> {
			BUILDER_FIELD(float, offset);
			BUILDER_FIELD_DEFAULT(bool, useWholePixels, false);

			Builder(Value<float> offset) : _offset(std::move(offset)) {}

			Shift* build() override {
				return new Shift(buildInner(), _offset, _useWholePixels.get());
			}
		};

	protected:
		Shift(LedPipelineStage* stage, Value<float> offset, bool useWholePixels = false);

		float effectiveOffset() const;
	};
}
