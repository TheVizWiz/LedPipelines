#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
	struct Solid : LedPipelineStage {
		RGBA color;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<Solid, Builder> {
			BUILDER_FIELD(RGBA, color);

			explicit Builder(const RGBA color) : _color(color) {}

			Solid* build() override {
				return new Solid(_color, _blendingMode);
			}
		};

	protected:
		Solid(RGBA color, BlendingMode blendingMode = BlendingMode::NORMAL);
	};


	struct SolidSegment : Solid {
		float length;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<SolidSegment, Builder> {
			BUILDER_FIELD(RGBA, color);
			BUILDER_FIELD(float, length);
			explicit Builder(const RGBA color, const float length) : _color(color), _length(length) {}

			SolidSegment* build() override {
				return new SolidSegment(_color, _length, _blendingMode);
			}
		};

	protected:
		SolidSegment(const RGBA color, float length, BlendingMode blendingMode = BlendingMode::NORMAL);
	};
} // namespace ledpipelines::effects
