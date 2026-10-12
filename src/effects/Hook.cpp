
#include <effects/Hook.h>


namespace ledpipelines::effects {


	void Hook::calculate(float startIndex, TemporaryLedData& tempData) {
		WrapperEffect::calculate(startIndex, tempData);
		if (!callback.get()) return;
		callback.get()({this->state});
	}

#ifdef LEDPIPELINES_SERIALIZATION
	std::string Hook::toJson(bool includeState) {
		std::string result = "{";
		result += jsonHeader("Hook");
		result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
		if (includeState) result += jsonStateObject(jsonStateHeader());
		result += "}";
		return result;
	}
#endif


	void TimedHook::calculate(float startIndex, TemporaryLedData& tempData) {
		WrapperEffect::calculate(startIndex, tempData);
		if (!callback.get()) return;

		for (LedPipelineStage* current = stage; current; current = current->innerStage()) {
			if (TimedEffect* timer = current->timedEffect()) {
				callback.get()({{this->state}, timer->elapsedPercentage, timer->runtimeMs.get()});
				return;
			}
		}
		callback.get()({{this->state}, 0, 0});
	}

#ifdef LEDPIPELINES_SERIALIZATION
	std::string TimedHook::toJson(bool includeState) {
		std::string result = "{";
		result += jsonHeader("TimedHook");
		result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
		if (includeState) result += jsonStateObject(jsonStateHeader());
		result += "}";
		return result;
	}
#endif


	void StateHook::calculate(float startIndex, TemporaryLedData& tempData) {
		LedPipelineRunningState oldState = this->state;
		WrapperEffect::calculate(startIndex, tempData);
		notifyIfChanged(oldState);
	}

	void StateHook::reset() {
		LedPipelineRunningState oldState = this->state;
		WrapperEffect::reset();
		notifyIfChanged(oldState);
	}

	void StateHook::notifyIfChanged(LedPipelineRunningState oldState) {
		if (oldState == this->state || !callback.get()) return;
		callback.get()({oldState, this->state});
	}

#ifdef LEDPIPELINES_SERIALIZATION
	std::string StateHook::toJson(bool includeState) {
		std::string result = "{";
		result += jsonHeader("StateHook");
		result += jsonRawField("stage", this->stage ? this->stage->toJson(includeState) : "null");
		if (includeState) result += jsonStateObject(jsonStateHeader());
		result += "}";
		return result;
	}
#endif

}
