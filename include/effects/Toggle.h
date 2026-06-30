#pragma once


#include "BaseEffect.h"


namespace ledpipelines::effects {
	struct Toggle : public WrapperEffect {
		bool isActive;

		void deactivate();

		void activate();

		void toggle();

		void reset() override;

		void calculate(float startIndex, TemporaryLedData &tempData) override;

		struct Builder : WrapperEffect::Builder<Toggle> {
			Builder() {};

			Toggle *build() override {
				return new Toggle(_stage);
			}
		};

		private:
			explicit Toggle(LedPipelineStage *stage);
	};
}
