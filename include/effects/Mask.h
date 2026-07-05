#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Mask : LedPipelineStage {
		LedPipelineStage *base;
		LedPipelineStage *mask;
		bool useMaskRuntime;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		~Mask() override;

		struct Builder : LedPipelineStage::Builder<Mask, Builder> {
			BUILDER_FIELD(LedPipelineStage *, base);
			BUILDER_FIELD(LedPipelineStage *, mask);
			BUILDER_FIELD_DEFAULT(bool, useMaskRuntime, false);

			Builder(LedPipelineStage *base, LedPipelineStage *mask) : _base(base), _mask(mask) {};

			Mask *create() override {
				return new Mask(
					_base,
					_mask,
					_useMaskRuntime,
					_blendingMode
				);
			}
		};

		private:
			Mask(
				LedPipelineStage *base,
				LedPipelineStage *mask,
				bool useMaskRuntime,
				BlendingMode blendingMode = BlendingMode::NORMAL);
	};
}
