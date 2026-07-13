#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
	struct Solid : LedPipelineStage {
		RGBA color;
		uint8_t opacity;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : LedPipelineStage::Builder<Solid, Builder> {
			BUILDER_FIELD(RGBA, color);
			BUILDER_FIELD_DEFAULT(uint8_t, opacity, 0xFF);

			explicit Builder(const RGBA color) : _color(color) {}

			Solid* build() override {
				return new Solid(_color, _opacity, _blendingMode);
			}
		};

	protected:
		Solid(RGBA color, uint8_t opacity, BlendingMode blendingMode = BlendingMode::NORMAL);
	};


	struct SolidSegment : Solid {
		float length;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : LedPipelineStage::Builder<SolidSegment, Builder> {
			BUILDER_FIELD(RGBA, color);
			BUILDER_FIELD_DEFAULT(uint8_t, opacity, 0xFF);
			BUILDER_FIELD(float, length);
			explicit Builder(const RGBA color, const float length) : _color(color), _length(length) {}

			SolidSegment* build() override {
				return new SolidSegment(_color, _opacity, _length, _blendingMode);
			}
		};

	protected:
		SolidSegment(const RGBA color, uint8_t opacity, float length, BlendingMode blendingMode = BlendingMode::NORMAL);
	};
} // namespace ledpipelines::effects
