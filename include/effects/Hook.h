//
// Created by thevizwiz on 10/10/2026.
//


#pragma once
#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Hook : WrapperEffect {

		struct CallbackArgs {
			const LedPipelineRunningState state;
		};
		using Callback = std::function<void(CallbackArgs)>;


		Value<Callback> callback;

		Hook(LedPipelineStage* stage, Value<Callback> callback) : WrapperEffect(stage), callback(std::move(callback)) {}

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif


		struct Builder : WrapperEffect::Builder<Hook, Builder> {

			BUILDER_FIELD(Callback, callback);

			Hook* build() override {
				return new Hook(buildInner(), _callback);
			};
		};
	};


	struct TimedHook : WrapperEffect {
		struct CallbackArgs : Hook::CallbackArgs {
			const float elapsedPercentage;
			const unsigned long runtimeMs;
		};

		using Callback = std::function<void(CallbackArgs)>;
		Value<Callback> callback;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		TimedHook(LedPipelineStage* stage, Value<Callback> callback)
			: WrapperEffect(stage), callback(std::move(callback)) {}


#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif


		struct Builder : WrapperEffect::Builder<TimedHook, Builder> {
			BUILDER_FIELD(Callback, callback);

			TimedHook* build() override {
				return new TimedHook(buildInner(), _callback);
			};
		};
	};
} // namespace ledpipelines::effects

