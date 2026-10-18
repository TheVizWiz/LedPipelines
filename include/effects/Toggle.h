#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Toggle : public WrapperEffect {
		bool isActive;
		bool completeOnDeactivate;
		bool resetOnActivate;

		/**
		 * Stops rendering the inner. With completeOnDeactivate, also reports DONE immediately; otherwise the state stays
		 * frozen at its last value. No-op if already inactive.
		 */
		void deactivate();

		/**
		 * Resumes rendering the inner. With resetOnActivate - or always with completeOnDeactivate, since the stage
		 * already reported DONE - the inner is reset and restarts from the beginning; otherwise it continues and this
		 * stage mirrors its state again. No-op if already active.
		 */
		void activate();

		void toggle();

		void reset() override;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : WrapperEffect::Builder<Toggle, Builder> {
			BUILDER_FIELD_DEFAULT(bool, completeOnDeactivate, false);
			BUILDER_FIELD_DEFAULT(bool, resetOnActivate, true);

			Builder() {};

			Toggle* create() override {
				return new Toggle(buildInner(), _completeOnDeactivate.get(), _resetOnActivate.get());
			}
		};

	private:
		Toggle(LedPipelineStage* stage, bool completeOnDeactivate, bool resetOnActivate);
	};
}
