#pragma once

#include "BaseLedPipeline.h"
#include "Value.h"  // Value<RGBA> - the color input may be a constant or a randomized color

namespace ledpipelines::effects {
	struct Solid : LedPipelineStage {
		// The color: a Value<RGBA> recipe that caches its own sampled draw (see Value - the Value IS the cache, so no
		// separate sampledColor member). A plain RGBA is a constant; a RandomColor/RandomColorSet re-rolls on reset().
		// calculate() renders color.value; reset() re-samples it.
		Value<RGBA> color;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		// The builder's color field is a Value<RGBA> (the BUILDER_FIELD macro wraps the given type in Value): a plain
		// RGBA converts implicitly (constant), or pass a RandomColor / RandomColorSet for a randomized color. Same slot,
		// no RandomSolid needed.
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
		// Like color, length is a Value<float> that caches its own draw: a plain float is constant, or a RandomRange
		// gives a randomized segment length re-rolled on reset(). calculate() reads length.value.
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
} // namespace ledpipelines::effects
