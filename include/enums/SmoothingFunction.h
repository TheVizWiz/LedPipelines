#pragma once

namespace ledpipelines {
	class SmoothingFunction {
	public:
		enum SmoothingFunction_ { SMOOTH_LINEAR, LINEAR, SINE, QUADRATIC, INVERSE_QUADRATIC };

		SmoothingFunction(const SmoothingFunction_& function) : value(function) {}

		SmoothingFunction(const SmoothingFunction_&& function) : value(function) {}

		SmoothingFunction(const SmoothingFunction& function) : value(function.value) {}

		SmoothingFunction(const SmoothingFunction&& function) noexcept : value(function.value) {}

		SmoothingFunction& operator=(const SmoothingFunction&);

		float operator()(float percentage) const {
			return this->operator()(percentage, 0, 1);
		}

		float operator()(float percentage, float max) const {
			return this->operator()(percentage, 0, max);
		}

		float operator()(float percentage, float min, float max) const {
			return this->operator()(percentage, 0, 1, min, max);
		}

		float operator()(float amount, float oldMin, float oldMax, float newMin, float newMax) const;

#ifdef LEDPIPELINES_SERIALIZATION
		SmoothingFunction_ preset() const {
			return value;
		}

		const char* name() const {
			switch (value) {
			case SMOOTH_LINEAR: return "SMOOTH_LINEAR";
			case LINEAR: return "LINEAR";
			case SINE: return "SINE";
			case QUADRATIC: return "QUADRATIC";
			case INVERSE_QUADRATIC: return "INVERSE_QUADRATIC";
			}
			return "LINEAR";
		}
#endif

	private:
		SmoothingFunction_ value;
	};
}
