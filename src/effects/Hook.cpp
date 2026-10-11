
#include <effects/Hook.h>


namespace ledpipelines::effects {


	void Hook::calculate(float startIndex, TemporaryLedData& tempData) {
		WrapperEffect::calculate(startIndex, tempData);
		CallbackArgs args = {.state = this->state};
		callback.value(args);
	}

	std::string Hook::toJson(bool includeState) {}


	void TimedHook::calculate(float startIndex, TemporaryLedData& tempData) {
		WrapperEffect::calculate(startIndex, tempData);

		if (auto* timedEffect = dynamic_cast<TimedEffect*>(stage)) {
			CallbackArgs args = {this->state, timedEffect->elapsedPercentage, timedEffect->runtimeMs.value};
			callback.value(args);
		} else {
			CallbackArgs args = {this->state, 0, 0};
			callback.value(args);
		}
	}

	std::string TimedHook::toJson(bool includeState) {}

} // namespace ledpipelines::effects
