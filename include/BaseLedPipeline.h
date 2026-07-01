#pragma once

#include "FastLED.h"
#include "LedPipelineUtils.h"
#include "memory"

#define BUILDER_FIELD(type, field)									\
    type _##field;													\
    Builder& field(type v) { this->_##field = v; return *this; }

#define BUILDER_FIELD_DEFAULT(type, field, defaultValue)			\
	type _##field = defaultValue;									\
	Builder& field(type v) { this->_##field = v; return *this; }

namespace ledpipelines {
	enum class LedPipelineRunningState {
		NOT_STARTED,
		RUNNING,
		DONE
	};

	namespace effects {
		struct WrapperEffect;
	}


	struct LedPipelineStage {
		/**
		 * Base builder for all stages. The second template parameter, ConcreteBuilder, is the leaf builder type that
		 * derives from this one (CRTP). Setters defined here return ConcreteBuilder& so that fluent chaining keeps the
		 * leaf type, letting base-level and leaf-level setters be called in any order.
		 */
		template<typename T, typename ConcreteBuilder> struct Builder {
			virtual ~Builder() = default;

			BlendingMode _blendingMode = BlendingMode::NORMAL;

			ConcreteBuilder &blendingMode(BlendingMode v) {
				this->_blendingMode = v;
				return static_cast<ConcreteBuilder &>(*this);
			}

			virtual T *build() = 0;
		};

		LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;

		BlendingMode blendingMode;


		virtual void calculate(float startIndex, TemporaryLedData &tempData) = 0;

		virtual void run();

		virtual void reset();

		explicit LedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

		virtual ~LedPipelineStage();

		/**
		 * Wrap this stage in a WrapperEffect, configured via its Builder. The builder's inner stage is set to this
		 * stage, and the wrapper is built and returned. This allows linear, outside-in chaining of wrapper effects,
		 * e.g. stage->wrap(Moving::Builder(1000))->wrap(Loop::Builder()).
		 */
		template<class B> auto wrap(B &&builder) -> decltype(builder.stage(this).build()) {
			return builder.stage(this).build();
		}

		private:
			u_int64_t lastUpdateTimeMicros;
	};


	/**
	 * Base abstract class for defining LED pipelines - the construct that can be run
	 * and show effects on a light strip.
	 */
	struct LedPipeline : LedPipelineStage {
		virtual LedPipeline *addStage(std::unique_ptr<LedPipelineStage> stage);

		virtual LedPipeline *addStage(LedPipelineStage *stage);

		void reset() override;

		explicit LedPipeline(BlendingMode mode);

		~LedPipeline() override;

		std::vector<std::unique_ptr<LedPipelineStage> > stages = std::vector<std::unique_ptr<LedPipelineStage> >();
	};


	/**
	 * A pipeline that runs all its stages in parallel. The stage that gets added first
	 * gets applied first, with other layers getting applied on top. The pipeline continues
	 * until all stages have completed.
	 */
	class ParallelLedPipeline : public LedPipeline {
		public:
			ParallelLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

			void calculate(float startIndex, TemporaryLedData &tempData) override;
	};

	/**
	 * A pipeline that blocks every stage before it moves on to the next one. The stage must be considered complete (the
	 * calculate function must return false) before it moves on to the next stage. This is useful for functions that need
	 * to happen one after another, not at the same time.
	 */
	class SeriesLedPipeline : public LedPipeline {
		public:
			explicit SeriesLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

			void calculate(float startIndex, TemporaryLedData &tempData) override;

			void reset() override;

		private:
			int currentStage = -1;
	};
}
