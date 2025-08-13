#pragma once


namespace ledpipelines {
class SmoothingFunction {

public:

    enum SmoothingFunction_ {
        SMOOTH_LINEAR,
        LINEAR,
        SINE,
        QUADRATIC,
        INVERSE_QUADRATIC
    };

    SmoothingFunction(const SmoothingFunction_ &function) : value(function) {}

    SmoothingFunction(const SmoothingFunction_ &&function) : value(function) {}

    SmoothingFunction(const SmoothingFunction &function) : value(function.value) {}

    SmoothingFunction(const SmoothingFunction &&function) : value(function.value) {}


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


private:
    SmoothingFunction_ value;

};

}
