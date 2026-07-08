#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	// ResetBlocker wraps an inner effect and guards it against being reset mid-run. A normal reset() propagates to the
	// inner unconditionally; ResetBlocker only forwards the reset to the inner if the inner has already completed
	// (state == DONE). If the inner is still running, the reset is withheld from it - the ResetBlocker resets its own
	// state (so its calculate() re-initializes next frame) but leaves the inner mid-run to finish its current pass.
	//
	// This is useful under a Loop (or any parent that resets its children each cycle): it lets a sub-effect run to
	// completion before it can be restarted, rather than being cut off when the loop restarts. Note the flip side - if
	// the inner never reaches DONE on its own, a Loop around a ResetBlocker will never restart it.
	//
	// It carries no timing of its own and is a straight pass-through in calculate(), so it derives from WrapperEffect
	// (not TimedEffect). The reset policy is the whole point of the effect.
	struct ResetBlocker : public WrapperEffect {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : WrapperEffect::Builder<ResetBlocker, Builder> {
			Builder() = default;

			ResetBlocker* build() override {
				return new ResetBlocker(buildInner());
			}
		};

	private:
		explicit ResetBlocker(LedPipelineStage* stage);
	};
} // namespace ledpipelines::effects
