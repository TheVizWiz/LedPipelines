#pragma once

#include "BaseEffect.h"
#include "Value.h"

namespace ledpipelines::effects {
	/**
	 * Source effect that writes a color gradient between two positions on the strip, optionally animated over time.
	 * The RGB-space twin of HSVGradient: channels are interpolated directly, so the blend goes straight through the
	 * RGB midpoint (red -> blue passes through a dim purple) instead of around the color wheel.
	 *
	 * The color at a given (pixel, moment) is a bilinear interpolation across four RGBA corners - two axes, each with a
	 * start and end:
	 *
	 *   - POSITION axis: startPosition .. endPosition along the strip.
	 *   - TIME axis:     the effect's own timeline, 0 .. runtimeMs.
	 *
	 * The corners are grouped as two spatial gradients, one per time endpoint:
	 *
	 *   - startGradient = the look at t=0:       (startGradientStart at startPosition) -> (startGradientEnd at endPosition)
	 *   - endGradient   = the look at t=runtime: (endGradientStart at startPosition) -> (endGradientEnd at endPosition)
	 *
	 * Set only startGradient for a static gradient; set both endpoints of a gradient to the same color for a solid
	 * segment that crossfades color over time. When the timeline reaches runtimeMs the effect holds on endGradient and
	 * reports DONE (wrap in Loop to repeat).
	 *
	 * Position is always interpolated linearly; the TIME axis honors a SmoothingFunction (default LINEAR).
	 *
	 * startPosition and endPosition may be given in either order. Pixels outside [start, end] are left untouched,
	 * so effects below show through.
	 *
	 * runtimeMs defaults to 0, in which case the effect renders startGradient statically and never advances.
	 */
	struct RGBGradient : LedPipelineStage, TimedEffect {
		// Every input is sampled once per lifecycle and re-sampled on reset().
		Value<float> startPosition;
		Value<float> endPosition;

		Value<RGBA> startGradientStart;
		Value<RGBA> startGradientEnd;

		Value<RGBA> endGradientStart;
		Value<RGBA> endGradientEnd;

		Value<SmoothingFunction> smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<RGBGradient, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD(float, startPosition);
			BUILDER_FIELD(float, endPosition);
			BUILDER_FIELD_DEFAULT(RGBA, startGradientStart, RGBA(0, 0, 0));
			BUILDER_FIELD_DEFAULT(RGBA, startGradientEnd, RGBA(0, 0, 0));
			BUILDER_FIELD_DEFAULT(RGBA, endGradientStart, RGBA(0, 0, 0));
			BUILDER_FIELD_DEFAULT(RGBA, endGradientEnd, RGBA(0, 0, 0));
			BUILDER_FIELD_DEFAULT(SmoothingFunction, smoothingFunction, SmoothingFunction::LINEAR);

			// A static gradient: no timeline.
			Builder(Value<float> startPosition, Value<float> endPosition)
				: TimedEffect::Builder<Builder>(0ul), _startPosition(std::move(startPosition)),
				  _endPosition(std::move(endPosition)) {}

			Builder(Value<float> startPosition, Value<float> endPosition, Value<unsigned long> runtimeMs)
				: TimedEffect::Builder<Builder>(std::move(runtimeMs)), _startPosition(std::move(startPosition)),
				  _endPosition(std::move(endPosition)) {}

			// Also seeds endGradient to the same pair, so a caller that never sets endGradient() gets a static gradient
			// rather than a morph toward the default (black) corners. The rvalue overload returns by value so a chain
			// can be captured with auto without hitting the deleted copy constructor.
			Builder& startGradient(Value<RGBA> atStart, Value<RGBA> atEnd) & {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			Builder startGradient(Value<RGBA> atStart, Value<RGBA> atEnd) && {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return std::move(*this);
			}

			Builder& endGradient(Value<RGBA> atStart, Value<RGBA> atEnd) & {
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			Builder endGradient(Value<RGBA> atStart, Value<RGBA> atEnd) && {
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
					_blendingMode
				));
			}
		};

	protected:
		RGBGradient(
			Value<float> startPosition,
			Value<float> endPosition,
			Value<unsigned long> runtimeMs,
			Value<RGBA> startGradientStart,
			Value<RGBA> startGradientEnd,
			Value<RGBA> endGradientStart,
			Value<RGBA> endGradientEnd,
			Value<SmoothingFunction> smoothingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
}
