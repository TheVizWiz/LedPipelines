#pragma once

#include "FastLED.h"
#include "LedPipelineUtils.h"
#include "memory"
#include <type_traits>

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
			Builder() = default;

			virtual ~Builder() = default;

			// Move-only. A built builder caches a raw pointer to its product (_built) that it does not own; copying
			// would duplicate that pointer into two builders, each of which could hand the same stage to a wrapper or
			// pipeline and cause a double-free. Moving transfers the cache to a single owner instead.
			Builder(const Builder &) = delete;
			Builder &operator=(const Builder &) = delete;
			Builder(Builder &&) = default;
			Builder &operator=(Builder &&) = default;

			BlendingMode _blendingMode = BlendingMode::NORMAL;

			ConcreteBuilder &blendingMode(BlendingMode v) {
				this->_blendingMode = v;
				return static_cast<ConcreteBuilder &>(*this);
			}

			/**
			 * Build the product of this builder. Memoized: the first call delegates to create() (implemented by the
			 * leaf builder) and caches the result; every subsequent call returns that same instance. This makes build()
			 * idempotent, so handing the same builder to addStage() twice, or calling build() after a wrap() already
			 * built it, yields one object rather than several that would alias/double-free a shared inner stage. The
			 * builder does NOT own the returned pointer - ownership passes to the wrapper/pipeline that receives it.
			 *
			 * NOTE: Each builder can be built once. Calling build() more than once on a builder will return the same
			 * stage that was originally built, regardless of any settings changes on the builder.
			 */
			T *build() {
				if (_built == nullptr) _built = this->create();
				return _built;
			}

			/**
			 * Wrap the product of this builder in a WrapperEffect, staying in "builder land": builds this stage,
			 * attaches it to the wrapper builder, and returns that wrapper builder (not a built stage). This lets wrap()
			 * chains compose entirely with '.' and defer the final build() to the end (or to addStage), e.g.
			 * Solid::Builder(CRGB::Red).wrap(OpacityGradient::Builder(4)).wrap(Loop::Builder()).build().
			 * The return type is deduced so it is only resolved when wrap() is called (where T is complete), not at
			 * class-instantiation time.
			 */
			template<class B> auto wrap(B &&wrapper) {
				return this->build()->wrap(static_cast<B &&>(wrapper));
			}

			protected:
				// Actually construct the product. Implemented by each leaf builder (return new T(...)); callers use the
				// memoizing build() above rather than calling this directly.
				virtual T *create() = 0;

				// Cached result of the first build(); nullptr until then. Not owned by the builder.
				T *_built = nullptr;
		};

		LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;

		BlendingMode blendingMode;


		virtual void calculate(float startIndex, TemporaryLedData &tempData) = 0;

		virtual void run();

		virtual void reset();

		explicit LedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

		virtual ~LedPipelineStage();

		/**
		 * Wrap this stage in a WrapperEffect, configured via its Builder. The wrapper builder's inner stage is set to
		 * this stage, and the wrapper builder itself is returned (by value) - NOT built. Returning the builder keeps
		 * the chain in "builder land" so more setters or wrap()s can follow with '.', deferring build() to the end,
		 * e.g. stage->wrap(Moving::Builder(1000)).wrap(Loop::Builder()).build().
		 *
		 * The wrapper builder is moved into the return value (builders are move-only). We move unconditionally rather
		 * than forward, because the argument is often an lvalue: fluent setters like Moving::Builder(1000).startPos(0)
		 * return Builder&, so B deduces to an lvalue reference. Moving is safe here since the argument is always a
		 * transient in a build-once chain, never a builder reused afterward.
		 */
		template<class B> auto wrap(B &&builder) -> std::remove_reference_t<B> {
			builder.stage(this);
			return std::move(builder);
		}

		private:
			u_int64_t lastUpdateTimeMicros;
	};


	/**
	 * Base abstract class for defining LED pipelines - the construct that can be run
	 * and show effects on a light strip.
	 */
	struct LedPipeline : LedPipelineStage {
		/**
		 * Base builder for pipelines. Extends LedPipelineStage::Builder (so blendingMode() and CRTP chaining carry
		 * over) and accumulates child stages. Concrete pipelines derive a leaf Builder that constructs the pipeline
		 * and hands over the collected stages in build().
		 */
		template<typename T, typename ConcreteBuilder>
		struct Builder : LedPipelineStage::Builder<T, ConcreteBuilder> {
			std::vector<std::unique_ptr<LedPipelineStage> > _stages;

			// Add an already-built stage - the result of a wrap() chain, a bare Effect::Builder(...).build(), or a
			// nested pipeline's build(). Takes ownership.
			ConcreteBuilder &addStage(LedPipelineStage *stage) {
				_stages.push_back(std::unique_ptr<LedPipelineStage>(stage));
				return static_cast<ConcreteBuilder &>(*this);
			}

			// Convenience overloads: accept another builder directly and build it here, so callers can drop in a bare
			// Effect::Builder(...) (or a stored wrap()-chain builder) without a trailing .build(). Both lvalue and
			// rvalue builders are accepted, since wrap() results are often stored in locals before being added.
			template<typename U, typename V>
			ConcreteBuilder &addStage(LedPipelineStage::Builder<U, V> &builder) {
				return addStage(builder.build());
			}

			template<typename U, typename V>
			ConcreteBuilder &addStage(LedPipelineStage::Builder<U, V> &&builder) {
				return addStage(builder.build());
			}
		};

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
			struct Builder : LedPipeline::Builder<ParallelLedPipeline, Builder> {
				ParallelLedPipeline *create() override {
					auto *pipeline = new ParallelLedPipeline(this->_blendingMode);
					for (auto &stage : _stages) pipeline->addStage(std::move(stage));
					return pipeline;
				}
			};

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
			struct Builder : LedPipeline::Builder<SeriesLedPipeline, Builder> {
				SeriesLedPipeline *create() override {
					auto *pipeline = new SeriesLedPipeline(this->_blendingMode);
					for (auto &stage : _stages) pipeline->addStage(std::move(stage));
					return pipeline;
				}
			};

			explicit SeriesLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

			void calculate(float startIndex, TemporaryLedData &tempData) override;

			void reset() override;

		private:
			int currentStage = -1;
	};
}
