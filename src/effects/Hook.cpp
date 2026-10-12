
#include <effects/Hook.h>


namespace ledpipelines::effects {


	void Hook::calculate(float startIndex, TemporaryLedData& tempData) {
		WrapperEffect::calculate(startIndex, tempData);
		if (!callback.value) return;
		callback.value({this->state});
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
		if (!callback.value) return;

		LedPipelineStage* current = stage;
		while (current) {
			if (auto* timedEffect = dynamic_cast<TimedEffect*>(current)) {
				callback.value({{this->state}, timedEffect->elapsedPercentage, timedEffect->runtimeMs.value});
				return;
			}
			auto* wrapper = dynamic_cast<WrapperEffect*>(current);
			current = wrapper ? wrapper->innerStage() : nullptr;
		}
		callback.value({{this->state}, 0, 0});
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
		if (oldState == this->state || !callback.value) return;
		callback.value({oldState, this->state});
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
