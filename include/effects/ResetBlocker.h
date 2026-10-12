#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	// Guards an inner effect against being reset mid-run: reset() is only forwarded to the inner if it is already DONE.
	// If the inner is still running, only the ResetBlocker's own state is reset and the inner finishes its pass.
	//
	// Useful under a Loop: lets a sub-effect run to completion rather than being cut off when the loop restarts. If the
	// inner never reaches DONE on its own, a Loop around a ResetBlocker will never restart it.
	struct ResetBlocker : public WrapperEffect {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<ResetBlocker, Builder> {
			Builder() = default;

			ResetBlocker* build() override {
				return new ResetBlocker(buildInner());
			}
		};

	private:
		explicit ResetBlocker(LedPipelineStage* stage);
	};
}
