#pragma once

#include <memory>

#include "BaseEffect.h"

namespace ledpipelines::effects {
	/**
	 * Wraps an already-built stage held through a std::shared_ptr, letting the SAME stage instance appear in more than
	 * one place in the pipeline tree. Unlike other wrappers, build() does not rebuild the inner; the shared_ptr's
	 * reference count manages its lifetime. Does not derive from WrapperEffect, whose raw `delete stage` destructor
	 * would double-free a shared instance.
	 *
	 * IMPORTANT - per-run state (RUNNING/DONE, timer) is shared too. Fine for stateless sources (Solid, SolidSegment,
	 * static HSVGradient); a FOOTGUN for stateful effects (Moving, Loop, Spawner, any timed effect), whose call sites
	 * would stomp each other's state within a frame. Share stateless sub-effects only.
	 *
	 * Usage:
	 *
	 *   auto dot = std::shared_ptr<LedPipelineStage>(SolidSegment::Builder(RGBA::White, 3).build());
	 *   pipeline = ParallelLedPipeline::Builder()
	 *       .addStage(Shared::Builder(dot).shift(0))
	 *       .addStage(Shared::Builder(dot).shift(20))
	 *       .build();
	 */
	struct Shared : LedPipelineStage {
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<Shared, Builder> {
			std::shared_ptr<LedPipelineStage> _stage;

			explicit Builder(std::shared_ptr<LedPipelineStage> stage) : _stage(std::move(stage)) {}

			Shared* build() override {
				return new Shared(_stage, _blendingMode);
			}
		};

	protected:
		std::shared_ptr<LedPipelineStage> stage;

		explicit Shared(std::shared_ptr<LedPipelineStage> stage, BlendingMode blendingMode = BlendingMode::NORMAL);
	};
}
