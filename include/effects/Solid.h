#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
	struct Solid : LedPipelineStage {
		CRGB color;
		uint8_t opacity;


		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : LedPipelineStage::Builder<Solid> {
			BUILDER_FIELD(CRGB, color);
			BUILDER_FIELD_DEFAULT(uint8_t, opacity, 0xFF);

			explicit Builder(const CRGB color) : _color(color) {}

			Solid *build() override {
				return new Solid(
					_color,
					_opacity
				);
			}
		};

		protected:
			Solid(CRGB color, uint8_t opacity);
	};


	struct SolidSegment : Solid {
		float length;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : LedPipelineStage::Builder<SolidSegment> {
			BUILDER_FIELD(CRGB, color);
			BUILDER_FIELD_DEFAULT(uint8_t, opacity, 0xFF);
			BUILDER_FIELD(float, length);
			explicit Builder(const CRGB color, const float length) : _color(color), _length(length) {}

			SolidSegment *build() override {
				return new SolidSegment(
					_color,
					_opacity,
					_length
				);
			}
		};

		protected:
			SolidSegment(
				const CRGB color,
				uint8_t opacity,
				float length
			);
	};
}
