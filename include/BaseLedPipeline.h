#pragma once

#include <type_traits>
#include "FastLED.h"
#include "LedPipelineUtils.h"
#include "memory"

// Fluent field + setter. The setter is ref-qualified so capturing a chain works without std::move:
//   - called on an lvalue (a named builder):   returns Builder&,  chaining mutates in place (no copy)
//   - called on an rvalue (a temporary chain): returns Builder    by moving *this out, so `auto b =
//   X::Builder(..).f(..)`
//     move-constructs a value instead of trying to copy the (deleted-copy, move-only) builder.
#define BUILDER_FIELD(type, field)                                                                                     \
	type _##field;                                                                                                     \
	Builder& field(type v) & {                                                                                         \
		this->_##field = v;                                                                                            \
		return *this;                                                                                                  \
	}                                                                                                                  \
	Builder field(type v) && {                                                                                         \
		this->_##field = v;                                                                                            \
		return std::move(*this);                                                                                       \
	}

#define BUILDER_FIELD_DEFAULT(type, field, defaultValue)                                                               \
	type _##field = defaultValue;                                                                                      \
	Builder& field(type v) & {                                                                                         \
		this->_##field = v;                                                                                            \
		return *this;                                                                                                  \
	}                                                                                                                  \
	Builder field(type v) && {                                                                                         \
		this->_##field = v;                                                                                            \
		return std::move(*this);                                                                                       \
	}

// CRTP variants of the above, for the standalone mixin builders (TimedEffect::Builder, RandomTimedEffect::Builder)
// that are templated on the leaf builder type. Unlike BUILDER_FIELD, whose setters return the enclosing `Builder`,
// these return `ConcreteBuilder` (the leaf, via a static_cast of *this) so a chain keeps the leaf type and can call
// leaf-level setters afterwards. Requires the enclosing scope to have a `ConcreteBuilder` template parameter.
#define BUILDER_FIELD_CRTP(type, field)                                                                                \
	type _##field;                                                                                                     \
	ConcreteBuilder& field(type v) & {                                                                                 \
		this->_##field = v;                                                                                            \
		return static_cast<ConcreteBuilder&>(*this);                                                                   \
	}                                                                                                                  \
	ConcreteBuilder field(type v) && {                                                                                 \
		this->_##field = v;                                                                                            \
		return std::move(static_cast<ConcreteBuilder&>(*this));                                                        \
	}

#define BUILDER_FIELD_CRTP_DEFAULT(type, field, defaultValue)                                                          \
	type _##field = defaultValue;                                                                                      \
	ConcreteBuilder& field(type v) & {                                                                                 \
		this->_##field = v;                                                                                            \
		return static_cast<ConcreteBuilder&>(*this);                                                                   \
	}                                                                                                                  \
	ConcreteBuilder field(type v) && {                                                                                 \
		this->_##field = v;                                                                                            \
		return std::move(static_cast<ConcreteBuilder&>(*this));                                                        \
	}

namespace ledpipelines {
	enum class LedPipelineRunningState { NOT_STARTED, RUNNING, DONE };

	namespace effects {
		struct WrapperEffect;

		// Forward-declared so the base Builder below can offer convenience wrappers (loop(), timebox(), shift(),
		// block()) for these common effects. The methods are member templates defined out-of-line in
		// effects/BuilderConveniences.h, which is included only after these types are complete - so the base builder
		// stays free of a hard dependency on the effect headers (which include this one).
		struct Loop;
		struct TimeBox;
		struct Shift;
		struct ResetBlocker;
	}

	struct LedPipelineStage;


	/**
	 * Non-template base shared by every stage builder. LedPipelineStage::Builder<T, ConcreteBuilder> is a template, so
	 * distinct effects produce unrelated builder types with no common base to store polymorphically. This base erases
	 * the product type down to LedPipelineStage*, letting one builder hold another (e.g. a WrapperEffect builder
	 * holding its inner effect's builder) via unique_ptr<StageBuilder> without knowing the concrete builder type.
	 */
	struct StageBuilder {
		StageBuilder() = default;

		virtual ~StageBuilder() = default;

		// Declaring the destructor suppresses the implicit move members; redeclare them so derived builders (which are
		// returned by value from wrap()) stay movable rather than silently falling back to (deleted) copy.
		StageBuilder(StageBuilder&&) = default;
		StageBuilder& operator=(StageBuilder&&) = default;
		StageBuilder(const StageBuilder&) = default;
		StageBuilder& operator=(const StageBuilder&) = default;

		// Build the product as a base LedPipelineStage*. Implemented by Builder<T, ConcreteBuilder> by delegating to
		// its typed build().
		virtual LedPipelineStage* buildStage() = 0;
	};


	struct LedPipelineStage {
		/**
		 * Base builder for all stages. The second template parameter, ConcreteBuilder, is the leaf builder type that
		 * derives from this one (CRTP). Setters defined here return ConcreteBuilder& so that fluent chaining keeps the
		 * leaf type, letting base-level and leaf-level setters be called in any order.
		 */
		template <typename T, typename ConcreteBuilder>
		struct Builder : StageBuilder {
			Builder() = default;

			~Builder() override = default;

			// Move-only. Builders own move-only state (a wrapper builder owns its inner builder via
			// unique_ptr<StageBuilder>, a pipeline builder owns a vector of unique_ptr stages), and wrap() moves
			// builders into one another. Copying is meaningless here and would try to duplicate that owned state.
			Builder(const Builder&) = delete;
			Builder& operator=(const Builder&) = delete;
			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			BlendingMode _blendingMode = BlendingMode::NORMAL;

			// Ref-qualified like the BUILDER_FIELD setters: lvalue chains in place (returns ConcreteBuilder&), rvalue
			// temporary moves out by value (returns ConcreteBuilder) so a chain can be captured without std::move.
			ConcreteBuilder& blendingMode(BlendingMode v) & {
				this->_blendingMode = v;
				return static_cast<ConcreteBuilder&>(*this);
			}

			ConcreteBuilder blendingMode(BlendingMode v) && {
				this->_blendingMode = v;
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

			/**
			 * Build the product of this builder - implemented by each leaf builder (return new T(...)). Each call
			 * returns a brand-new, independent stage; the builder is a factory, not a cache. Calling build() twice
			 * yields two separate trees that share nothing, so each is owned and deleted exactly once by whoever
			 * receives it. For wrappers, this rebuilds the inner effect too (see wrap()), so no stage is ever shared
			 * across builds. The builder does NOT own the returned pointer; ownership passes to the receiver.
			 *
			 * NOTE: A builder can be built any number of times; each build() reflects the builder's current settings
			 * and produces a fresh, fully independent stage.
			 */
			virtual T* build() = 0;

			// Type-erased build() for the StageBuilder base: returns the product as a LedPipelineStage*. Lets code
			// holding a StageBuilder (e.g. a wrapper holding its inner builder) build without the concrete type.
			LedPipelineStage* buildStage() override {
				return build();
			}

			/**
			 * Wrap this builder in a WrapperEffect's builder, staying entirely in "builder land": nothing is built
			 * here. This builder is moved onto the heap and adopted as the wrapper's inner builder; the wrapper builder
			 * is then returned by value so more setters or wrap()s can follow with '.', deferring build() to the end
			 * (or to addStage), e.g.
			 * Solid::Builder(CRGB::Red).wrap(OpacityGradient::Builder(4)).wrap(Loop::Builder()).build().
			 *
			 * Because the inner is stored as a builder (not a built stage), each build() of the resulting chain
			 * produces a fresh, independent tree. The wrapper argument is moved (builders are move-only); it is
			 * consumed by this call. B is deduced (often an lvalue ref, since fluent setters return Builder&), so we
			 * move via a remove_reference cast rather than forward.
			 */
			template <class B>
			auto wrap(B&& wrapper) -> std::remove_reference_t<B> {
				// Move this builder (the concrete leaf type, via CRTP) onto the heap so the wrapper can own it through
				// the type-erased StageBuilder base.
				std::unique_ptr<StageBuilder> inner(
					new ConcreteBuilder(std::move(static_cast<ConcreteBuilder&>(*this)))
				);
				wrapper.innerBuilder(std::move(inner));
				return std::move(wrapper);
			}

			/**
			 * Convenience shorthands for the most common wrap() targets - e.g. `.loop()` instead of
			 * `.wrap(Loop::Builder())`. Each just forwards to wrap() with the corresponding effect's builder, so the
			 * result is identical to writing the wrap() out by hand (and can be chained with more setters/wraps).
			 *
			 * Only declared here; defined out-of-line in effects/BuilderConveniences.h (included last, after the effect
			 * headers). Their bodies name concrete effect builders (Loop, TimeBox, ...) that are only forward-declared at
			 * this point, but since Builder is itself a class template its member definitions are instantiated lazily on
			 * use - by which point those types are complete. That deferral is why the base builder needs no hard include
			 * dependency on the effect headers, and why these need no dummy template parameter.
			 */
			auto loop();                        // wrap(Loop::Builder()) - loops forever
			auto loop(size_t numLoops);         // wrap(Loop::Builder().numLoops(n))
			auto timebox(unsigned long runtimeMs); // wrap(TimeBox::Builder(runtimeMs))
			auto shift(float numPixels);        // wrap(Shift::Builder(numPixels))
			auto block();                       // wrap(ResetBlocker::Builder())
		};

		LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;

		BlendingMode blendingMode;


		virtual void calculate(float startIndex, TemporaryLedData& tempData) = 0;

		virtual void run();

		virtual void reset();

		explicit LedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

		virtual ~LedPipelineStage();

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
		template <typename T, typename ConcreteBuilder>
		struct Builder : LedPipelineStage::Builder<T, ConcreteBuilder> {
			// Child stages stored as builders (not built stages), mirroring how WrapperEffect::Builder defers its
			// inner. build() rebuilds each child fresh into a new pipeline, so building a pipeline builder more than
			// once yields fully independent trees rather than moving a fixed set of stages out on the first build.
			std::vector<std::unique_ptr<StageBuilder>> _childBuilders;

			// Add a child stage builder. Accepts any builder (a bare Effect::Builder(...), a wrap()-chain builder, or a
			// nested pipeline builder). The child is moved onto the heap and owned via the type-erased StageBuilder
			// base; it is built fresh each time this pipeline's create() runs.
			//
			// Ref-qualified like the other fluent setters: on an lvalue pipeline builder the call chains in place
			// (returns ConcreteBuilder&); on an rvalue temporary it moves the pipeline builder out by value (returns
			// ConcreteBuilder) so a chain ending in addStage can be captured without std::move. Each also accepts the
			// child by lvalue or rvalue, since wrap() results are often stored in locals first.
			template <typename U, typename V>
			ConcreteBuilder& addStage(LedPipelineStage::Builder<U, V>& builder) & {
				adoptChild<U, V>(builder);
				return static_cast<ConcreteBuilder&>(*this);
			}

			template <typename U, typename V>
			ConcreteBuilder& addStage(LedPipelineStage::Builder<U, V>&& builder) & {
				return addStage(builder);
			}

			template <typename U, typename V>
			ConcreteBuilder addStage(LedPipelineStage::Builder<U, V>& builder) && {
				adoptChild<U, V>(builder);
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

			template <typename U, typename V>
			ConcreteBuilder addStage(LedPipelineStage::Builder<U, V>&& builder) && {
				return std::move(*this).addStage(builder);
			}

		private:
			// Move a child builder onto the heap and adopt it (type-erased). Shared by the ref-qualified overloads.
			// V is the concrete leaf builder type; builder is typed as the base, so cast down before moving.
			template <typename U, typename V>
			void adoptChild(LedPipelineStage::Builder<U, V>& builder) {
				_childBuilders.push_back(std::unique_ptr<StageBuilder>(new V(std::move(static_cast<V&>(builder)))));
			}
		};

		virtual LedPipeline* addStage(std::unique_ptr<LedPipelineStage> stage);

		virtual LedPipeline* addStage(LedPipelineStage* stage);

		void reset() override;

		explicit LedPipeline(BlendingMode mode);

		~LedPipeline() override;

		std::vector<std::unique_ptr<LedPipelineStage>> stages = std::vector<std::unique_ptr<LedPipelineStage>>();
	};


	/**
	 * A pipeline that runs all its stages in parallel. The stage that gets added first
	 * gets applied first, with other layers getting applied on top. The pipeline continues
	 * until all stages have completed.
	 */
	class ParallelLedPipeline : public LedPipeline {
	public:
		struct Builder : LedPipeline::Builder<ParallelLedPipeline, Builder> {
			ParallelLedPipeline* build() override {
				auto* pipeline = new ParallelLedPipeline(this->_blendingMode);
				for (auto& childBuilder : _childBuilders) pipeline->addStage(childBuilder->buildStage());
				return pipeline;
			}
		};

		ParallelLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

		void calculate(float startIndex, TemporaryLedData& tempData) override;
	};

	/**
	 * A pipeline that blocks every stage before it moves on to the next one. The stage must be considered complete (the
	 * calculate function must return false) before it moves on to the next stage. This is useful for functions that
	 * need to happen one after another, not at the same time.
	 */
	class SeriesLedPipeline : public LedPipeline {
	public:
		struct Builder : LedPipeline::Builder<SeriesLedPipeline, Builder> {
			SeriesLedPipeline* build() override {
				auto* pipeline = new SeriesLedPipeline(this->_blendingMode);
				for (auto& childBuilder : _childBuilders) pipeline->addStage(childBuilder->buildStage());
				return pipeline;
			}
		};

		explicit SeriesLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

	private:
		int currentStage = -1;
	};
} // namespace ledpipelines
