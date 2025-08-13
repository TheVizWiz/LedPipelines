#pragma once

#include "math.h"

namespace ledpipelines {
class SamplingFunction {
public:

    enum SamplingFunction_ {
        UNIFORM,
        CENTERED,
    };

    SamplingFunction(const SamplingFunction_ &function) : value(function) {};

    SamplingFunction(const SamplingFunction_ &&function) : value(function) {};

    SamplingFunction(const SamplingFunction &function) : value(function.value) {};

    SamplingFunction(const SamplingFunction &&function) : value(function.value) {};

    float operator()() const { return this->operator()(0, 1); }

    float operator()(float max) const { return this->operator()(0, max); };

    float operator()(float min, float max) const;

private:
    enum SamplingFunction_ value;

    /**
     * Values used in computation
     */
    const float tanh_min = tanh(-2.5);
    const float tanh_max = tanh(2.5);
    const float tanh_range = tanh_max - tanh_min;

};

}