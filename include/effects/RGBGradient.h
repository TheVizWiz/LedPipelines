#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {
	/**
	 * RGBGradient is a source effect that writes a color gradient between two positions on the strip, and can
	 * optionally animate that gradient over time. It is the RGB-space twin of HSVGradient: where HSVGradient
	 * interpolates hue and therefore travels around the color wheel (red -> blue passes through magenta or through
	 * green), RGBGradient interpolates the R, G, B channels directly, so the blend goes straight between the two colors
	 * through their RGB midpoint (red -> blue passes through a dim purple). Use RGBGradient when you want a plain
	 * smooth crossfade between two colors; use HSVGradient when you want the intermediate hues of the wheel.
	 *
	 * The color at a given (pixel, moment) is a bilinear interpolation across four CRGB corners - two axes, each with a
	 * start and end:
	 *
	 *   - POSITION axis: startPosition .. endPosition along the strip.
	 *   - TIME axis:     the effect's own timeline, 0 .. runtimeMs.
	 *
	 * Rather than four separately named corners, the corners are grouped as two spatial gradients, one per time
	 * endpoint:
	 *
	 *   - startGradient = the look at t=0:      (startGradientStart at startPosition) -> (startGradientEnd at
	 * endPosition)
	 *   - endGradient   = the look at t=runtime: (endGradientStart at startPosition) -> (endGradientEnd at endPosition)
	 *
	 * So the effect is "gradient A morphing into gradient B over time." Set only startGradient for a static gradient;
	 * set both endpoints of a gradient to the same color for a solid segment that crossfades color over time. When the
	 * timeline reaches runtimeMs the effect holds on endGradient and reports DONE (wrap in Loop to repeat).
	 *
	 * Both axes interpolate linearly per channel. Position is always linear; the TIME axis honors a SmoothingFunction
	 * (default LINEAR), matching Moving.
	 *
	 * startPosition and endPosition may be given in either order. Pixels outside [start, end] are left untouched,
	 * exactly like SolidSegment, so effects below show through.
	 *
	 * runtimeMs defaults to 0, in which case the effect renders startGradient statically and never advances.
	 */
	struct RGBGradient : LedPipelineStage, TimedEffect {
		float startPosition;
		float endPosition;

		// The gradient at t=0.
		CRGB startGradientStart;
		CRGB startGradientEnd;

		// The gradient at t=runtimeMs. When these equal the startGradient corners, the gradient is static.
		CRGB endGradientStart;
		CRGB endGradientEnd;

		SmoothingFunction smoothingFunction;
		uint8_t opacity;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<RGBGradient, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD(float, startPosition);
			BUILDER_FIELD(float, endPosition);
			BUILDER_FIELD_DEFAULT(CRGB, startGradientStart, CRGB(0, 0, 0));
			BUILDER_FIELD_DEFAULT(CRGB, startGradientEnd, CRGB(0, 0, 0));
			BUILDER_FIELD_DEFAULT(CRGB, endGradientStart, CRGB(0, 0, 0));
			BUILDER_FIELD_DEFAULT(CRGB, endGradientEnd, CRGB(0, 0, 0));
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);
			BUILDER_FIELD_DEFAULT(uint8_t, opacity, 0xFF);

			// A static gradient: no timeline. endGradient defaults to matching startGradient (see startGradient()).
			Builder(float startPosition, float endPosition)
				: TimedEffect::Builder<Builder>(0), _startPosition(startPosition), _endPosition(endPosition) {}

			// A time-animated gradient: give the runtime up front, then set startGradient()/endGradient().
			Builder(float startPosition, float endPosition, unsigned long runtimeMs)
				: TimedEffect::Builder<Builder>(runtimeMs), _startPosition(startPosition), _endPosition(endPosition) {}

			// Set the gradient shown at t=0 (start-of-position color -> end-of-position color). Also seeds endGradient
			// to the same pair, so a caller that never sets endGradient() gets a static gradient rather than a morph
			// toward the default (black) corners. Ref-qualified like the BUILDER_FIELD setters: lvalue chains in place
			// (returns Builder&), rvalue temporary moves out by value (returns Builder) so a chain can be captured with
			// auto without hitting the deleted copy constructor.
			Builder& startGradient(CRGB atStart, CRGB atEnd) & {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			Builder startGradient(CRGB atStart, CRGB atEnd) && {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return std::move(*this);
			}

			// Set the gradient morphed toward at t=runtimeMs (start-of-position color -> end-of-position color).
			Builder& endGradient(CRGB atStart, CRGB atEnd) & {
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			Builder endGradient(CRGB atStart, CRGB atEnd) && {
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return std::move(*this);
			}

			RGBGradient* build() override {
				return applyTiming(new RGBGradient(
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
		RGBGradient(
			float startPosition,
			float endPosition,
			unsigned long runtimeMs,
			CRGB startGradientStart,
			CRGB startGradientEnd,
			CRGB endGradientStart,
			CRGB endGradientEnd,
			SmoothingFunction smoothingFunction,
			uint8_t opacity,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
} // namespace ledpipelines::effects
