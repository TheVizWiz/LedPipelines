#pragma once

// Out-of-line definitions for the convenience wrapper methods declared on LedPipelineStage::Builder in
// BaseLedPipeline.h (loop(), timebox(), shift(), block()). They are defined here, rather than inline in the base, so
// that the concrete effect builders they reference are fully visible - this header is included only at the end of
// LedEffects.h, after every effect header. Because Builder is a class template, these member definitions are only
// instantiated when actually called, so the forward declarations in BaseLedPipeline.h are enough there. Each method
// just forwards to wrap() with the matching effect builder, so `builder.loop()` is exactly `builder.wrap(Loop::Builder())`.

#include "Loop.h"
#include "Shift.h"
#include "TimeBox.h"
#include "ResetBlocker.h"

namespace ledpipelines {
	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::loop() {
		return wrap(effects::Loop::Builder());
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::loop(size_t numLoops) {
		return wrap(effects::Loop::Builder().numLoops(numLoops));
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::timebox(unsigned long runtimeMs) {
		return wrap(effects::TimeBox::Builder(runtimeMs));
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::shift(float numPixels) {
		return wrap(effects::Shift::Builder(numPixels));
	}

	template <class T, class ConcreteBuilder>
	auto LedPipelineStage::Builder<T, ConcreteBuilder>::block() {
		return wrap(effects::ResetBlocker::Builder());
	}
}
