#pragma once


#include "BaseEffect.h"
#include "Value.h"  // Value<float> - the offset may be a constant or a randomized position (RandomRange)

namespace ledpipelines::effects {
	// Shifts its inner effect along the strip by an offset. The offset is a Value<float> that caches its own sampled
	// draw (see Value - the Value IS the cache): a plain number for a fixed shift, or a RandomRange for a randomized
	// position (this replaces the old RandomShift - the offset is sampled once per lifecycle and re-rolled on reset(),
	// same as every other Value input). useWholePixels snaps the effective offset to an integer pixel so the shifted
	// copy lands exactly on LEDs (no fractional-coverage feathering); off by default.
	struct Shift : WrapperEffect {
		// The offset RECIPE that caches its draw for the current lifecycle. calculate() shifts by effectiveOffset()
		// (offset.value, rounded when useWholePixels). reset() re-samples it.
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
				return new Shift(buildInner(), _offset, _useWholePixels.value);
			}
		};

	protected:
		Shift(LedPipelineStage* stage, Value<float> offset, bool useWholePixels = false);

		// The offset actually applied this lifecycle: the cached sample, snapped to a whole pixel when useWholePixels.
		float effectiveOffset() const;
	};
} // namespace ledpipelines::effects
