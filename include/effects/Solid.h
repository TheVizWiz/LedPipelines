#pragma once

#include "BaseLedPipeline.h"
#include "Value.h"

namespace ledpipelines::effects {
	struct Solid : LedPipelineStage {
		Value<RGBA> color;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<Solid, Builder> {
			BUILDER_FIELD(RGBA, color);

			explicit Builder(Value<RGBA> color) : _color(std::move(color)) {}

			Solid* build() override {
				return new Solid(_color, _blendingMode);
			}
		};

	protected:
		Solid(Value<RGBA> color, BlendingMode blendingMode = BlendingMode::NORMAL);
	};


	struct SolidSegment : Solid {
		Value<float> length;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<SolidSegment, Builder> {
			BUILDER_FIELD(RGBA, color);
			BUILDER_FIELD(float, length);
			explicit Builder(Value<RGBA> color, Value<float> length) : _color(std::move(color)), _length(std::move(length)) {}

			SolidSegment* build() override {
				return new SolidSegment(_color, _length, _blendingMode);
			}
		};

	protected:
		SolidSegment(Value<RGBA> color, Value<float> length, BlendingMode blendingMode = BlendingMode::NORMAL);
	};
}
