#pragma once


#include "BaseEffect.h"
#include "enums/SamplingFunction.h"

namespace ledpipelines::effects {
	struct Shift : WrapperEffect {
		float offset;

		void calculate(float startIndex, TemporaryLedData& tempData) override;


		struct Builder : WrapperEffect::Builder<Shift, Builder> {
			BUILDER_FIELD(float, offset);

			Builder(float offset) : _offset(offset) {}

			Shift* build() override {
				return new Shift(buildInner(), _offset);
			}
		};

	protected:
		Shift(LedPipelineStage* stage, float offset);
	};


	// Like Shift, but the offset is sampled once from [minOffset, maxOffset] the first time the effect runs, rather than
	// fixed at build time. reset() returns the effect to NOT_STARTED, so a re-run (e.g. inside a Loop) re-samples a fresh
	// offset - mirroring how the random timed effects re-roll their duration on each run. Not a RandomTimedEffect: the
	// randomized quantity here is a position, not a duration, so there is no timer involved.
	struct RandomShift : WrapperEffect {
		float minOffset;
		float maxOffset;
		SamplingFunction samplingFunction;

		// The offset sampled for the current run. Set on the first calculate() after a reset; undefined until then.
		float offset;

		// Whether offset has been sampled for the current run. Tracked explicitly (rather than keying off state) so an
		// inner that never advances past NOT_STARTED - e.g. a bare Solid - doesn't cause a fresh sample every frame.
		// reset() clears this so the next run re-samples.
		bool sampled = false;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;


		struct Builder : WrapperEffect::Builder<RandomShift, Builder> {
			BUILDER_FIELD_DEFAULT(float, minOffset, 0);
			BUILDER_FIELD(float, maxOffset);
			BUILDER_FIELD_DEFAULT(SamplingFunction, samplingFunction, SamplingFunction::UNIFORM);

			Builder(float maxOffset) : _maxOffset(maxOffset) {}

			RandomShift* build() override {
				return new RandomShift(buildInner(), _minOffset, _maxOffset, _samplingFunction);
			}
		};

	protected:
		RandomShift(LedPipelineStage* stage, float minOffset, float maxOffset, SamplingFunction samplingFunction);
	};
} // namespace ledpipelines::effects
