#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Arduino.h"

#include "Handle.h"

#include "TemporaryLedData.h"
#include "enums/BlendingMode.h"
#include "enums/SamplingFunction.h"
#include "enums/SmoothingFunction.h"
#include "Value.h"
#include "serialization/JsonFields.h"

namespace ledpipelines {
	// Minimum spacing between rendered frames, in microseconds (0 = no limit). Set via setMaxRefreshRate().
	extern uint64_t minMicrosBetweenUpdates;

	/**
	 * Set the max refresh rate of LedPipelines. Defaults to no max refresh rate. This is NOT blocking: run() calls
	 * made sooner than the interval are ignored. Updates are not guaranteed at this interval, only spaced at least
	 * this far apart.
	 * @param refreshesPerSecond the new max refresh rate, in refreshes / second.
	 */
	void setMaxRefreshRate(float refreshesPerSecond);
}

// Fluent field + setter. The field is a Value<type>, so the setter accepts a plain `type`, a provider (e.g.
// RandomColor()), or an existing Value<type>.
//
// The setter is ref-qualified so capturing a chain works without std::move: on an lvalue it returns Builder& and
// mutates in place; on an rvalue it returns Builder by moving *this out, so `auto b = X::Builder(..).f(..)` works.
#define BUILDER_FIELD(type, field)                                                                                     \
	Value<type> _##field;                                                                                              \
	Builder& field(Value<type> v) & {                                                                                  \
		this->_##field = std::move(v);                                                                                 \
		return *this;                                                                                                  \
	}                                                                                                                  \
	Builder field(Value<type> v) && {                                                                                  \
		this->_##field = std::move(v);                                                                                 \
		return std::move(*this);                                                                                       \
	}

#define BUILDER_FIELD_DEFAULT(type, field, defaultValue)                                                               \
	Value<type> _##field = defaultValue;                                                                               \
	Builder& field(Value<type> v) & {                                                                                  \
		this->_##field = std::move(v);                                                                                 \
		return *this;                                                                                                  \
	}                                                                                                                  \
	Builder field(Value<type> v) && {                                                                                  \
		this->_##field = std::move(v);                                                                                 \
		return std::move(*this);                                                                                       \
	}

// CRTP variants of the above, for mixin builders templated on the leaf builder type (e.g. TimedEffect::Builder).
// Setters return `ConcreteBuilder` so a chain keeps the leaf type. Requires a `ConcreteBuilder` template parameter in
// the enclosing scope.
#define BUILDER_FIELD_CRTP(type, field)                                                                                \
	Value<type> _##field;                                                                                              \
	ConcreteBuilder& field(Value<type> v) & {                                                                          \
		this->_##field = std::move(v);                                                                                 \
		return static_cast<ConcreteBuilder&>(*this);                                                                   \
	}                                                                                                                  \
	ConcreteBuilder field(Value<type> v) && {                                                                          \
		this->_##field = std::move(v);                                                                                 \
		return std::move(static_cast<ConcreteBuilder&>(*this));                                                        \
	}

#define BUILDER_FIELD_CRTP_DEFAULT(type, field, defaultValue)                                                          \
	Value<type> _##field = defaultValue;                                                                               \
	ConcreteBuilder& field(Value<type> v) & {                                                                          \
		this->_##field = std::move(v);                                                                                 \
		return static_cast<ConcreteBuilder&>(*this);                                                                   \
	}                                                                                                                  \
	ConcreteBuilder field(Value<type> v) && {                                                                          \
		this->_##field = std::move(v);                                                                                 \
		return std::move(static_cast<ConcreteBuilder&>(*this));                                                        \
	}

namespace ledpipelines {
	enum class LedPipelineRunningState { NOT_STARTED, RUNNING, DONE };

	namespace effects {
		struct WrapperEffect;
		struct TimedEffect;

		// Forward-declared for the Builder convenience wrappers (loop(), timebox(), ...), which are defined in
		// effects/BuilderConveniences.h; the effect headers include this one, so they can't be included here.
		struct Loop;
		struct TimeBox;
		struct Shift;
		struct ResetBlocker;
	}

	struct LedPipelineStage;

	/**
	 * Type-erased, non-template base of every stage builder, so one builder can own another (e.g. a WrapperEffect
	 * builder's inner builder) via unique_ptr<StageBuilder> without knowing its concrete type.
	 */
	struct StageBuilder {
		StageBuilder() = default;

		virtual ~StageBuilder() = default;

		// Declaring the destructor suppresses the implicit move members; redeclare them so derived builders stay movable.
		StageBuilder(StageBuilder&&) = default;
		StageBuilder& operator=(StageBuilder&&) = default;
		StageBuilder(const StageBuilder&) = default;
		StageBuilder& operator=(const StageBuilder&) = default;

		virtual LedPipelineStage* buildStage() = 0;

		// Polymorphic deep copy; lets an lvalue builder be reused, with each use producing an independent tree.
		virtual std::unique_ptr<StageBuilder> clone() const = 0;
	};

	struct LedPipelineStage {
		/**
		 * Base builder for all stages. ConcreteBuilder is the leaf builder type deriving from this one (CRTP), so
		 * base-level setters keep the leaf type and can be chained in any order with leaf-level ones.
		 */
		template <typename T, typename ConcreteBuilder>
		struct Builder : StageBuilder {
			Builder() = default;

			~Builder() override = default;

			// Builders that own another builder via unique_ptr<StageBuilder> (WrapperEffect::Builder,
			// LedPipeline::Builder) must override the copy members to deep-clone it.
			Builder(const Builder&) = default;
			Builder& operator=(const Builder&) = default;
			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			std::unique_ptr<StageBuilder> clone() const override {
				return std::unique_ptr<StageBuilder>(new ConcreteBuilder(static_cast<const ConcreteBuilder&>(*this)));
			}

			BlendingMode _blendingMode = BlendingMode::NORMAL;

			std::vector<std::function<void(T*)>> _bindings;

			/**
			 * Register every stage this builder (or any copy of it) builds with `handle`. See Handle.
			 */
			template <typename U, typename = std::enable_if_t<std::is_base_of_v<U, T>>>
			ConcreteBuilder& bind(const Handle<U>& handle) & {
				this->_bindings.push_back([handle](T* product) { handle.attach(static_cast<U*>(product)); });
				return static_cast<ConcreteBuilder&>(*this);
			}

			template <typename U, typename = std::enable_if_t<std::is_base_of_v<U, T>>>
			ConcreteBuilder bind(const Handle<U>& handle) && {
				this->_bindings.push_back([handle](T* product) { handle.attach(static_cast<U*>(product)); });
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

			ConcreteBuilder& blendingMode(BlendingMode v) & {
				this->_blendingMode = v;
				return static_cast<ConcreteBuilder&>(*this);
			}

			ConcreteBuilder blendingMode(BlendingMode v) && {
				this->_blendingMode = v;
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

			/**
			 * Build a new, fully independent stage tree reflecting the builder's current settings (inner/child
			 * builders are rebuilt too). Can be called any number of times. Ownership of the returned pointer passes
			 * to the caller.
			 */
			T* build() {
				T* product = create();
				for (auto& bindProduct : _bindings) bindProduct(product);
				return product;
			}

			/**
			 * Construct this builder's product. Implemented by each leaf builder (return new T(...)); call build()
			 * instead, which also applies bind() registrations.
			 */
			virtual T* create() = 0;

			/**
			 * Like build(), but returns a shared_ptr. Each call creates a **new** stage, not another pointer to the
			 * same one.
			 */
			std::shared_ptr<LedPipelineStage> buildShared() {
				// Explicit template arg: std::shared_ptr has no deduction guide for a raw pointer.
				return std::shared_ptr<LedPipelineStage>(buildStage());
			}

			LedPipelineStage* buildStage() override {
				return build();
			}

			/**
			 * Adopt this builder as the inner builder of a WrapperEffect builder and return the wrapper builder;
			 * nothing is built. e.g. Solid::Builder(RGBA::Red).wrap(OpacityGradient::Builder(4)).wrap(Loop::Builder())
			 *
			 * On an lvalue, this builder is deep-copied so it can be reused (`ball.wrap(A); ball.wrap(B);` gives two
			 * independent trees); on an rvalue it is moved.
			 */
			template <class B>
			auto wrap(B&& wrapper) const& -> std::remove_reference_t<B> {
				std::unique_ptr<StageBuilder> inner = this->clone();
				wrapper.innerBuilder(std::move(inner));
				return std::move(wrapper);
			}

			template <class B>
			auto wrap(B&& wrapper) && -> std::remove_reference_t<B> {
				std::unique_ptr<StageBuilder> inner(
					new ConcreteBuilder(std::move(static_cast<ConcreteBuilder&>(*this))));
				wrapper.innerBuilder(std::move(inner));
				return std::move(wrapper);
			}

			/**
			 * Shorthands for common wrap() targets, e.g. `.loop()` for `.wrap(Loop::Builder())`. Defined in
			 * effects/BuilderConveniences.h; this works because Builder's members are only instantiated on use, after
			 * the effect types are complete.
			 */
			auto loop(); // wrap(Loop::Builder()) - loops forever
			auto loop(Value<size_t> numLoops); // wrap(Loop::Builder().numLoops(n))
			auto timebox(Value<unsigned long> runtimeMs); // wrap(TimeBox::Builder(runtimeMs))
			auto shift(Value<float> numPixels); // wrap(Shift::Builder(numPixels))
			auto block(); // wrap(ResetBlocker::Builder())
			auto shared(); // Shared::Builder(buildShared())
		};

		LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;

#ifdef LEDPIPELINES_SERIALIZATION
		// Random per-instance identifier emitted in serialized JSON.
		char id[10] = "123456789";
#endif

		BlendingMode blendingMode;

		virtual void calculate(float startIndex, TemporaryLedData& tempData) = 0;

		virtual void run();

		virtual void reset();

		/**
		 * This stage's timeline, or null if it is not a TimedEffect. Lets callers reach the timer without RTTI.
		 */
		virtual effects::TimedEffect* timedEffect() {
			return nullptr;
		}

		/**
		 * The stage this one wraps, or null if it does not wrap one.
		 */
		virtual LedPipelineStage* innerStage() const {
			return nullptr;
		}

		explicit LedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

		virtual ~LedPipelineStage();

#ifdef LEDPIPELINES_SERIALIZATION

		/**
		 * Shorthand for toJson(false): config only, no runtime state.
		 */
		std::string toJson() {
			return toJson(false);
		}

		/**
		 * Serialize this stage (and its children) to JSON. When includeState is true, runtime state is included under
		 * a "state" sub-object.
		 */
		virtual std::string toJson(bool includeState) = 0;

	protected:
		// First field of a "state" sub-object (the state analog of jsonHeader()). No leading comma; subsequent fields
		// add their own.
		std::string jsonStateHeader() {
			switch (state) {
			case LedPipelineRunningState::NOT_STARTED: return R"("runningState": "NOT_STARTED")";
			case LedPipelineRunningState::RUNNING: return R"("runningState": "RUNNING")";
			case LedPipelineRunningState::DONE: return R"("runningState": "DONE")";
			}
			return R"("runningState": "UNSPECIFIED")";
		}

		std::string jsonHeader(const std::string& type) {
			return R"("type": ")" + type + R"(", "id": ")" + std::string(id) + "\"";
		}

		// Forwarders to the shared free functions in serialization/JsonFields.h, so toJson() bodies need no
		// qualification.
		template <typename T>
		static std::string jsonField(const std::string& name, const T& value) {
			return ledpipelines::serialization::jsonField(name, value);
		}

		static std::string jsonRawField(const std::string& name, const std::string& rawJson) {
			return ledpipelines::serialization::jsonRawField(name, rawJson);
		}

		// Emits ,"state" : { <inner> }. `inner` must start with jsonStateHeader(), e.g.
		//   if (includeState) result += jsonStateObject(jsonStateHeader() + jsonTimedEffect() + ...);
		std::string jsonStateObject(const std::string& inner) {
			return jsonRawField("state", "{" + inner + "}");
		}

#endif

	private:
		uint64_t lastUpdateTimeMicros;

		std::vector<std::shared_ptr<detail::HandleRegistryBase>> handleRegistries;

		template <typename>
		friend class Handle;
	};

	template <typename T>
	template <typename Stage>
	void Handle<T>::attach(Stage* stage) const {
		registry->stages.push_back(stage);
		static_cast<LedPipelineStage*>(stage)->handleRegistries.push_back(registry);
	}

	/**
	 * Base abstract class for LED pipelines: stages that own and combine child stages.
	 */
	struct LedPipeline : LedPipelineStage {
		/**
		 * Base builder for pipelines; accumulates child stage builders, which concrete builders build fresh in
		 * create().
		 */
		template <typename T, typename ConcreteBuilder>
		struct Builder : LedPipelineStage::Builder<T, ConcreteBuilder> {
			std::vector<std::unique_ptr<StageBuilder>> _childBuilders;

			Builder() = default;
			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			// Deep copy: clones each child builder.
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

			// Add a child stage builder (any builder, including wrap() chains and nested pipelines). Like wrap(), an
			// lvalue child is deep-copied (so `p.addStage(x); q.addStage(x);` leaves x intact) and an rvalue child is
			// moved. *this chains in place on an lvalue and moves out on an rvalue.
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
			// V is the concrete leaf builder type; cast down so the whole leaf is copied/moved, not just the base.
			template <typename U, typename V>
			void adoptChildCopy(LedPipelineStage::Builder<U, V>& builder) {
				_childBuilders.push_back(std::unique_ptr<StageBuilder>(new V(static_cast<const V&>(builder))));
			}

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
			ParallelLedPipeline* create() override {
				auto* pipeline = new ParallelLedPipeline(this->_blendingMode);
				for (auto& childBuilder : _childBuilders) pipeline->addStage(childBuilder->buildStage());
				return pipeline;
			}
		};

		ParallelLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif
	};

	/**
	 * A pipeline that runs its stages one after another, advancing once the current stage's state is DONE.
	 */
	class SeriesLedPipeline : public LedPipeline {
	public:
		struct Builder : LedPipeline::Builder<SeriesLedPipeline, Builder> {
			SeriesLedPipeline* create() override {
				auto* pipeline = new SeriesLedPipeline(this->_blendingMode);
				for (auto& childBuilder : _childBuilders) pipeline->addStage(childBuilder->buildStage());
				return pipeline;
			}
		};

		explicit SeriesLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

	private:
		int currentStage = -1;
	};
}
