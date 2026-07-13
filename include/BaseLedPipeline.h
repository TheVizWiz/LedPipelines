#pragma once

// Standard headers used directly below (vector/shared_ptr members, std::remove_if, std::move, fixed-width ints).
// Included explicitly rather than relied on transitively: macOS/ESP32 toolchains pull these in via other headers, but
// stricter standard libraries (e.g. MinGW on Windows) do not, so omitting them breaks the Windows build.
#include <algorithm>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// The platform timing seam: millis()/micros()/delay(). Host-provided - real on Arduino/ESP32, stubbed by the viewer
// and the tests. run() (rate limiting) and every timed effect (BaseEffect.h) depend on it.
#include "Arduino.h"

// The core runtime types this header (and everything that includes it, e.g. BaseEffect.h) refers to. Previously pulled
// in transitively via the now-removed LedPipelineUtils.h; included directly so the dependency is explicit.
#include "TemporaryLedData.h"
#include "enums/BlendingMode.h"
#include "enums/SamplingFunction.h"
#include "enums/SmoothingFunction.h"

namespace ledpipelines {
	// The minimum spacing between rendered frames, in microseconds (0 = no limit). run() consults it to self-rate-limit
	// (see setMaxRefreshRate). Defined in BaseLedPipeline.cpp so there is a single shared instance across translation
	// units.
	extern uint64_t minMicrosBetweenUpdates;

	/**
	 * Set the max refresh rate of LedPipelines. Defaults to no max refresh rate. Note that this is NOT blocking in the
	 * same way that a hardware show() is blocking; if you call pipeline.run() more often than the max refresh rate, the
	 * extra calls will just be ignored until the next update is ready. An update is not *guaranteed* to take place at
	 * this interval, but it is guaranteed that it will take at *least* this interval, even if you try to update
	 * LedPipelines faster than this.
	 * @param refreshesPerSecond the new max refresh rate, in refreshes / second. e.g. 30fps, 50fps, 144fps.
	 */
	void setMaxRefreshRate(float refreshesPerSecond);
} // namespace ledpipelines

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
	} // namespace effects

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

		// Polymorphic deep copy. Builders own their inner/child builders through this type-erased base, so copying a
		// builder needs each concrete builder to clone itself (the base can't know the concrete type). Implemented by
		// Builder<T, ConcreteBuilder>. This is what lets a plain builder be reused: reusing an lvalue builder copies it
		// (via this) so the original stays intact and each use produces an independent tree.
		virtual std::unique_ptr<StageBuilder> clone() const = 0;
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

			// Copyable and movable. A builder is a reusable recipe: reusing one (e.g. wrapping the same builder into two
			// different parents) copies it so each use yields an independent tree, and the original stays valid. Leaf
			// builders copy member-wise via these defaults; builders that own another builder through a
			// unique_ptr<StageBuilder> (WrapperEffect::Builder's inner, LedPipeline::Builder's children) override the copy
			// members to deep-clone that owned builder rather than shallow-copy the pointer.
			Builder(const Builder&) = default;
			Builder& operator=(const Builder&) = default;
			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			// Polymorphic deep copy (see StageBuilder::clone). CRTP gives us the concrete leaf type, so we can copy the
			// whole leaf - which, for wrapper/pipeline builders, runs their deep-cloning copy constructor.
			std::unique_ptr<StageBuilder> clone() const override {
				return std::unique_ptr<StageBuilder>(new ConcreteBuilder(static_cast<const ConcreteBuilder&>(*this)));
			}

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

			/**
			 * Build a shared pointer to a new instance of an LedPipelingStage. If called multiple times, this method
			 * will create multiple **new** LedPipelineStages, **not** multiple shared pointers to multiple stages.
			 *
			 * @return a shared pointer to a newly constructed LedPipelineStage
			 */
			std::shared_ptr<LedPipelineStage> buildShared() {
				// Explicit template arg: std::shared_ptr has no deduction guide for a raw T*, so CTAD
				// (std::shared_ptr(ptr)) would not compile.
				return std::shared_ptr<LedPipelineStage>(buildStage());
			}

			// Type-erased build() for the StageBuilder base: returns the product as a LedPipelineStage*. Lets code
			// holding a StageBuilder (e.g. a wrapper holding its inner builder) build without the concrete type.
			LedPipelineStage* buildStage() override {
				return build();
			}

			/**
			 * Wrap this builder in a WrapperEffect's builder, staying entirely in "builder land": nothing is built here.
			 * This builder is adopted as the wrapper's inner builder; the wrapper builder is then returned by value so
			 * more setters or wrap()s can follow with '.', deferring build() to the end (or to addStage), e.g.
			 * Solid::Builder(RGBA::Red).wrap(OpacityGradient::Builder(4)).wrap(Loop::Builder()).build().
			 *
			 * Two overloads, so a builder is a reusable recipe:
			 *   - called on an LVALUE (a named builder): this builder is COPIED (deep clone) into the wrapper, leaving the
			 *     original intact so it can be reused - e.g. `ball.wrap(A); ball.wrap(B);` gives two independent trees.
			 *   - called on an RVALUE (a temporary chain, or an explicit std::move): this builder is MOVED into the
			 *     wrapper (no copy), since a temporary won't be reused.
			 * Either way the inner is stored as a builder (not a built stage), so each build() of the result produces a
			 * fresh, independent tree.
			 */
			template <class B>
			auto wrap(B&& wrapper) const& -> std::remove_reference_t<B> {
				// Lvalue: deep-copy this builder (via clone()) so the original stays usable for further reuse.
				std::unique_ptr<StageBuilder> inner = this->clone();
				wrapper.innerBuilder(std::move(inner));
				return std::move(wrapper);
			}

			template <class B>
			auto wrap(B&& wrapper) && -> std::remove_reference_t<B> {
				// Rvalue: move this builder (the concrete leaf type, via CRTP) onto the heap - no copy needed.
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
			 * headers). Their bodies name concrete effect builders (Loop, TimeBox, ...) that are only forward-declared
			 * at this point, but since Builder is itself a class template its member definitions are instantiated
			 * lazily on use - by which point those types are complete. That deferral is why the base builder needs no
			 * hard include dependency on the effect headers, and why these need no dummy template parameter.
			 */
			auto loop(); // wrap(Loop::Builder()) - loops forever
			auto loop(size_t numLoops); // wrap(Loop::Builder().numLoops(n))
			auto timebox(unsigned long runtimeMs); // wrap(TimeBox::Builder(runtimeMs))
			auto shift(float numPixels); // wrap(Shift::Builder(numPixels))
			auto block(); // wrap(ResetBlocker::Builder())
			auto shared(); // Shared::Builder(buildShared())
		};

		LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;

		BlendingMode blendingMode;


		virtual void calculate(float startIndex, TemporaryLedData& tempData) = 0;

		virtual void run();

		virtual void reset();

		explicit LedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

		virtual ~LedPipelineStage();

	private:
		uint64_t lastUpdateTimeMicros;
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

			Builder() = default;
			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			// Deep copy: clone each child builder rather than shallow-copying the (move-only) unique_ptrs, so copying a
			// pipeline builder yields an independent set of child sub-trees. This makes a pipeline builder reusable the
			// same way a wrapper-chain builder is.
			Builder(const Builder& other) : LedPipelineStage::Builder<T, ConcreteBuilder>(other) {
				_childBuilders.reserve(other._childBuilders.size());
				for (const auto& child : other._childBuilders) {
					_childBuilders.push_back(child ? child->clone() : nullptr);
				}
			}

			Builder& operator=(const Builder& other) {
				LedPipelineStage::Builder<T, ConcreteBuilder>::operator=(other);
				_childBuilders.clear();
				_childBuilders.reserve(other._childBuilders.size());
				for (const auto& child : other._childBuilders) {
					_childBuilders.push_back(child ? child->clone() : nullptr);
				}
				return *this;
			}

			// Add a child stage builder. Accepts any builder (a bare Effect::Builder(...), a wrap()-chain builder, or a
			// nested pipeline builder), owned via the type-erased StageBuilder base and built fresh each time this
			// pipeline's build() runs.
			//
			// Symmetric with wrap(), on BOTH the pipeline builder and the child, so a builder stays a reusable recipe:
			//   - the CHILD is COPIED when passed as an lvalue (a named builder, reusable afterwards) and MOVED when
			//     passed as an rvalue (a temporary / std::move) - so `p.addStage(x); q.addStage(x);` gives p and q
			//     independent children and leaves x intact.
			//   - `*this` (the pipeline builder) chains in place on an lvalue (returns ConcreteBuilder&) and moves out by
			//     value on an rvalue temporary (returns ConcreteBuilder), so a chain ending in addStage needs no
			//     std::move to be captured.
			template <typename U, typename V>
			ConcreteBuilder& addStage(LedPipelineStage::Builder<U, V>& builder) & {
				adoptChildCopy<U, V>(builder);
				return static_cast<ConcreteBuilder&>(*this);
			}

			template <typename U, typename V>
			ConcreteBuilder& addStage(LedPipelineStage::Builder<U, V>&& builder) & {
				adoptChildMove<U, V>(builder);
				return static_cast<ConcreteBuilder&>(*this);
			}

			template <typename U, typename V>
			ConcreteBuilder addStage(LedPipelineStage::Builder<U, V>& builder) && {
				adoptChildCopy<U, V>(builder);
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

			template <typename U, typename V>
			ConcreteBuilder addStage(LedPipelineStage::Builder<U, V>&& builder) && {
				adoptChildMove<U, V>(builder);
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

		private:
			// Adopt a child by COPYING it (deep clone), leaving the caller's builder intact for reuse. Used for lvalue
			// children. V is the concrete leaf builder type; builder is typed as the base, so cast down before copying.
			template <typename U, typename V>
			void adoptChildCopy(LedPipelineStage::Builder<U, V>& builder) {
				_childBuilders.push_back(std::unique_ptr<StageBuilder>(new V(static_cast<const V&>(builder))));
			}

			// Adopt a child by MOVING it (no copy). Used for rvalue children (temporaries / std::move).
			template <typename U, typename V>
			void adoptChildMove(LedPipelineStage::Builder<U, V>& builder) {
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
