//
// Created by thevizwiz on 10/10/2026.
//


#pragma once
#include <functional>

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Hook : WrapperEffect {

		struct CallbackArgs {
			const LedPipelineRunningState state;
		};
		using Callback = std::function<void(CallbackArgs)>;


		Value<Callback> callback;

		Hook(LedPipelineStage* stage, Value<Callback> callback) : WrapperEffect(stage), callback(std::move(callback)) {}

		/**
		 * Render the inner stage, then invoke the callback (if set) with the resulting state. Fires every frame.
		 */
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

		/**
		 * Render the inner stage, then invoke the callback (if set) with the resulting state and the timeline of the
		 * nearest TimedEffect found by walking down the chain of wrapped stages. If no TimedEffect is reachable through
		 * wrappers, elapsedPercentage and runtimeMs are 0. Fires every frame.
		 */
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


	struct StateHook : WrapperEffect {
		struct CallbackArgs {
			const LedPipelineRunningState oldState;
			const LedPipelineRunningState newState;
		};

		using Callback = std::function<void(CallbackArgs)>;
		Value<Callback> callback;

		StateHook(LedPipelineStage* stage, Value<Callback> callback)
			: WrapperEffect(stage), callback(std::move(callback)) {}

		/**
		 * Render the inner stage and invoke the callback (if set) only if the state changed during this frame. A frame
		 * that skips a state (e.g. NOT_STARTED straight to DONE) reports a single change.
		 */
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		/**
		 * Reset the inner stage and invoke the callback (if set) if the reset changed the state (e.g. DONE to
		 * NOT_STARTED).
		 */
		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif


		struct Builder : WrapperEffect::Builder<StateHook, Builder> {
			BUILDER_FIELD(Callback, callback);

			StateHook* build() override {
				return new StateHook(buildInner(), _callback);
			};
		};

	private:
		/**
		 * Invoke the callback if the state differs from oldState.
		 */
		void notifyIfChanged(LedPipelineRunningState oldState);
	};
} // namespace ledpipelines::effects
