#pragma once

#include "BaseEffect.h"
#include "Value.h"

namespace ledpipelines::effects {
	/**
	 * Source effect that writes a color gradient between two positions on the strip, optionally animated over time.
	 *
	 * The color at a given (pixel, moment) is a bilinear interpolation across four HSVA corners - two axes, each with a
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
	 * segment that cycles color over time. When the timeline reaches runtimeMs the effect holds on endGradient and
	 * reports DONE (wrap in Loop to repeat).
	 *
	 * Hue is interpolated linearly on its raw value on BOTH axes - going from h=0 to h=240 travels forward through the
	 * full spectrum, not the short way around the wheel; hue is only wrapped into [0, 360) at conversion, so h=0..720
	 * sweeps two full rainbows. Position is always interpolated linearly; the TIME axis honors a SmoothingFunction
	 * (default LINEAR).
	 *
	 * startPosition and endPosition may be given in either order. Pixels outside [start, end] are left untouched,
	 * so effects below show through.
	 *
	 * runtimeMs defaults to 0, in which case the effect renders startGradient statically and never advances.
	 */
	struct HSVGradient : LedPipelineStage, TimedEffect {
		// Every input is sampled once per lifecycle and re-sampled on reset().
		Value<float> startPosition;
		Value<float> endPosition;

		Value<HSVA> startGradientStart;
		Value<HSVA> startGradientEnd;

		Value<HSVA> endGradientStart;
		Value<HSVA> endGradientEnd;

		Value<SmoothingFunction> smoothingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<HSVGradient, Builder>, TimedEffect::Builder<Builder> {
			BUILDER_FIELD(float, startPosition);
			BUILDER_FIELD(float, endPosition);
			BUILDER_FIELD_DEFAULT(HSVA, startGradientStart, HSVA());
			BUILDER_FIELD_DEFAULT(HSVA, startGradientEnd, HSVA());
			BUILDER_FIELD_DEFAULT(HSVA, endGradientStart, HSVA());
			BUILDER_FIELD_DEFAULT(HSVA, endGradientEnd, HSVA());
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
			Builder& startGradient(Value<HSVA> atStart, Value<HSVA> atEnd) & {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			Builder startGradient(Value<HSVA> atStart, Value<HSVA> atEnd) && {
				this->_startGradientStart = atStart;
				this->_startGradientEnd = atEnd;
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return std::move(*this);
			}

			Builder& endGradient(Value<HSVA> atStart, Value<HSVA> atEnd) & {
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return *this;
			}

			Builder endGradient(Value<HSVA> atStart, Value<HSVA> atEnd) && {
				this->_endGradientStart = atStart;
				this->_endGradientEnd = atEnd;
				return std::move(*this);
			}

			HSVGradient* build() override {
				return applyTiming(new HSVGradient(
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
		HSVGradient(
			Value<float> startPosition,
			Value<float> endPosition,
			Value<unsigned long> runtimeMs,
			Value<HSVA> startGradientStart,
			Value<HSVA> startGradientEnd,
			Value<HSVA> endGradientStart,
			Value<HSVA> endGradientEnd,
			Value<SmoothingFunction> smoothingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
}
