#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Mask : LedPipelineStage {
		LedPipelineStage *base;
		LedPipelineStage *mask;
		bool useMaskRuntime;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<Mask> {
			BUILDER_FIELD(LedPipelineStage *, base);
			BUILDER_FIELD(LedPipelineStage *, mask);
			BUILDER_FIELD_DEFAULT(bool, useMaskRuntime, false);

			Builder(LedPipelineStage *base, LedPipelineStage *mask) : _base(base), _mask(mask) {};

			Mask *build() override {
				return new Mask(
					_base,
					_mask,
					_useMaskRuntime
				);
			}
		};

		private:
			Mask(
				LedPipelineStage *base,
				LedPipelineStage *mask,
				bool useMaskRuntime);
	};
}
