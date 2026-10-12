#pragma once

// Out-of-line definitions for the convenience wrapper methods declared on LedPipelineStage::Builder in
// BaseLedPipeline.h. Defined here so the concrete effect builders they reference are complete - this header must be
// included only at the end of LedEffects.h, after every effect header.

#include "Loop.h"
#include "Shift.h"
#include "TimeBox.h"
#include "ResetBlocker.h"
#include "Shared.h"

namespace ledpipelines {
	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::loop() {
		return wrap(effects::Loop::Builder());
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::loop(Value<size_t> numLoops) {
		return wrap(effects::Loop::Builder().numLoops(std::move(numLoops)));
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::timebox(Value<unsigned long> runtimeMs) {
		return wrap(effects::TimeBox::Builder(std::move(runtimeMs)));
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::shift(Value<float> numPixels) {
		return wrap(effects::Shift::Builder(std::move(numPixels)));
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::block() {
		return wrap(effects::ResetBlocker::Builder());
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::shared() {
		return effects::Shared::Builder(buildShared());
	}
}
