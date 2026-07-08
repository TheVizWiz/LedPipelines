#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	/**
	 * HSVGradient is a source effect that writes a color gradient between two positions on the strip, and can optionally
	 * animate that gradient over time.
	 *
	 * The color at a given (pixel, moment) is a bilinear interpolation across four FHSV corners - two axes, each with a
	 * start and end:
	 *
	 *   - POSITION axis: startPosition .. endPosition along the strip.
	 *   - TIME axis:     the effect's own timeline, 0 .. runtimeMs.
	 *
	 * Rather than four separately named corners, the corners are grouped as two spatial gradients, one per time endpoint:
	 *
	 *   - startGradient = the look at t=0:      (startGradientStart at startPosition) -> (startGradientEnd at endPosition)
	 *   - endGradient   = the look at t=runtime: (endGradientStart at startPosition) -> (endGradientEnd at endPosition)
	 *
	 * So the effect is "gradient A morphing into gradient B over time." Set only startGradient for a static gradient (the
	 * original behavior); set both endpoints of a gradient to the same color for a solid segment that cycles color over
	 * time. When the timeline reaches runtimeMs the effect holds on endGradient and reports DONE (wrap in Loop to repeat).
	 *
	 * Hue is interpolated linearly on its raw value on BOTH axes - going from h=0 to h=240 travels forward through the
	 * full spectrum, not the short way around the wheel; hue is only wrapped into [0, 360) at conversion, so h=0..720
	 * sweeps two full rainbows. Position is always interpolated linearly; the TIME axis honors a SmoothingFunction
	 * (default LINEAR), matching Moving.
	 *
	 * startPosition and endPosition may be given in either order. Pixels outside [start, end] are left untouched, exactly
	 * like SolidSegment, so effects below show through.
	 *
	 * A static (single-gradient) HSVGradient needs no timeline; it is only meaningfully "timed" once endGradient differs.
	 * runtimeMs defaults to 0, in which case the effect renders startGradient statically and never advances.
	 */
	struct HSVGradient : LedPipelineStage, TimedEffect {
		float startPosition;
		float endPosition;

		// The gradient at t=0.
		FHSV startGradientStart;
		FHSV startGradientEnd;

		// The gradient at t=runtimeMs. When these equal the startGradient corners, the gradient is static.
		FHSV endGradientStart;
		FHSV endGradientEnd;

		SmoothingFunction smoothingFunction;
		uint8_t opacity;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<HSVGradient, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD(float, startPosition);
			BUILDER_FIELD(float, endPosition);
			BUILDER_FIELD_DEFAULT(FHSV, startGradientStart, FHSV());
			BUILDER_FIELD_DEFAULT(FHSV, startGradientEnd, FHSV());
			BUILDER_FIELD_DEFAULT(FHSV, endGradientStart, FHSV());
			BUILDER_FIELD_DEFAULT(FHSV, endGradientEnd, FHSV());
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);
			BUILDER_FIELD_DEFAULT(uint8_t, opacity, 0xFF);

			// A static gradient: no timeline. endGradient defaults to matching startGradient (see startGradient()).
			Builder(float startPosition, float endPosition) :
				TimedEffect::Builder<Builder>(0),
				_startPosition(startPosition), _endPosition(endPosition) {}

			// A time-animated gradient: give the runtime up front, then set startGradient()/endGradient().
			Builder(float startPosition, float endPosition, unsigned long runtimeMs) :
				TimedEffect::Builder<Builder>(runtimeMs),
				_startPosition(startPosition), _endPosition(endPosition) {}

			// Set the gradient shown at t=0 (start-of-position color -> end-of-position color). Also seeds endGradient to
			// the same pair, so a caller that never sets endGradient() gets a static gradient rather than a morph toward
			// the default (black) corners.
			Builder &startGradient(FHSV atStart, FHSV atEnd) {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			// Set the gradient morphed toward at t=runtimeMs (start-of-position color -> end-of-position color).
			Builder &endGradient(FHSV atStart, FHSV atEnd) {
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			HSVGradient *build() override {
				return applyTiming(new HSVGradient(
					_startPosition,
					_endPosition,
					_runtimeMs,
					_startGradientStart,
					_startGradientEnd,
					_endGradientStart,
					_endGradientEnd,
					_smoothingFunction,
					_opacity,
					_blendingMode
				));
			}
		};

		protected:
			HSVGradient(
				float startPosition,
				float endPosition,
				unsigned long runtimeMs,
				FHSV startGradientStart,
				FHSV startGradientEnd,
				FHSV endGradientStart,
				FHSV endGradientEnd,
				SmoothingFunction smoothingFunction,
				uint8_t opacity,
				BlendingMode blendingMode = BlendingMode::NORMAL
			);
	};
}
