#pragma once

#include <memory>

#include "BaseEffect.h"

namespace ledpipelines::effects {
	/**
	 * Shared wraps an already-built stage held through a std::shared_ptr, letting the SAME stage instance appear in more
	 * than one place in the pipeline tree. Every other wrapper takes a Builder and rebuilds a fresh, independent inner on
	 * each build() (so nothing is ever shared); Shared is the deliberate exception - it takes a built stage and hands out
	 * that exact instance, with the shared_ptr's reference count managing its lifetime (it is freed once the last Shared
	 * referencing it is destroyed, NOT eagerly like a normal wrapper's `delete stage`).
	 *
	 * Because it does NOT derive from WrapperEffect, it avoids WrapperEffect's raw `delete stage` destructor, which would
	 * double-free a shared instance.
	 *
	 * IMPORTANT - shared MUTABLE state. A stage carries per-run state (its RUNNING/DONE flag, and for timed effects the
	 * start time and elapsed percentage). Sharing ONE instance means that state is shared too. This is fine, and the
	 * intended use, for STATELESS / idempotent sources - e.g. a Solid, SolidSegment, or static HSVGradient - which render
	 * the same thing regardless of how often calculate()/reset() is called. It is a FOOTGUN for STATEFUL effects: if a
	 * Moving, Loop, Spawner, or any timed effect is shared and rendered from two places in a single frame, the two call
	 * sites stomp each other's state (one advances the timer or resets it out from under the other), producing corrupted
	 * or nondeterministic animation. Share stateless sub-effects; do not share stateful ones.
	 *
	 * Usage: build the stage once, wrap it in a shared_ptr, and hand the same shared_ptr to as many Shared wrappers as
	 * you like:
	 *
	 *   auto dot = std::shared_ptr<LedPipelineStage>(SolidSegment::Builder(CRGB::White, 3).build());
	 *   pipeline = ParallelLedPipeline::Builder()
	 *       .addStage(Shared::Builder(dot).shift(0))
	 *       .addStage(Shared::Builder(dot).shift(20))
	 *       .build();
	 */
	struct Shared : LedPipelineStage {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		struct Builder : LedPipelineStage::Builder<Shared, Builder> {
			// The shared inner stage. Held as a shared_ptr (not a StageBuilder) precisely so build() reuses the same
			// instance instead of rebuilding a fresh one - that reuse is the whole point of Shared.
			std::shared_ptr<LedPipelineStage> _stage;

			explicit Builder(std::shared_ptr<LedPipelineStage> stage) : _stage(std::move(stage)) {}

			// No custom copy/move needed: _stage is a copyable shared_ptr, so the base's default copy (used when an
			// lvalue builder is reused) copies the pointer - both copies reference the SAME inner instance, which is
			// exactly Shared's purpose. clone() (inherited) copies the shared_ptr too, so reusing a Shared::Builder in
			// several branches shares one instance.

			Shared* build() override {
				return new Shared(_stage, _blendingMode);
			}
		};

	protected:
		std::shared_ptr<LedPipelineStage> stage;

		explicit Shared(std::shared_ptr<LedPipelineStage> stage, BlendingMode blendingMode = BlendingMode::NORMAL);
	};
} // namespace ledpipelines::effects
