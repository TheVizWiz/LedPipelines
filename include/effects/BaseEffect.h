#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
	struct WrapperEffect : LedPipelineStage {
		protected:
			LedPipelineStage *stage;

		public:
			explicit WrapperEffect(LedPipelineStage *stage);

			~WrapperEffect() override;

			void reset() override;

			template<typename T, typename ConcreteBuilder>
			class Builder : public LedPipelineStage::Builder<T, ConcreteBuilder> {
				public:
					Builder() = default;

					// Movable (wrap() returns wrapper builders by value); copy stays deleted via the base.
					Builder(Builder &&) = default;
					Builder &operator=(Builder &&) = default;

					// The builder for the inner (wrapped) effect. Stored as a builder rather than a built stage so the
					// inner effect is (re)built fresh each time this wrapper's create() runs - every build() produces an
					// independent inner tree, so nothing is shared across builds. Owned via unique_ptr<StageBuilder>
					// (the type-erased builder base) because the concrete inner builder type varies per effect.
					std::unique_ptr<StageBuilder> _innerBuilder;

					// Adopt the inner builder. Called by LedPipelineStage::Builder::wrap() when this wrapper wraps an
					// inner builder; ownership of the inner builder moves here.
					ConcreteBuilder &innerBuilder(std::unique_ptr<StageBuilder> inner) {
						this->_innerBuilder = std::move(inner);
						return static_cast<ConcreteBuilder &>(*this);
					}

				protected:
					// Build the inner effect as a fresh stage. Leaf create()s pass the result into their product's
					// constructor (the product then owns and deletes it). Returns nullptr if no inner was set.
					LedPipelineStage *buildInner() {
						return _innerBuilder ? _innerBuilder->buildStage() : nullptr;
					}

				public:
					T *create() override = 0;
			};
	};


	struct TimedEffect {\
		virtual ~TimedEffect() = default;

		unsigned long startTimeMs;
		float elapsedPercentage;
		unsigned long runtimeMs;

		virtual void resetTimer();

		/**
		 * Standalone mixin builder contributing timing fields. Deliberately does NOT derive from
		 * LedPipelineStage::Builder: leaf builders (e.g. Moving, FadeIn) inherit that base through their other builder
		 * parent, so inheriting it here too would create an ambiguous duplicate base. Templated on the leaf builder
		 * type (CRTP) so its setters return the leaf type and chain cleanly with other setters.
		 */
		template<typename ConcreteBuilder> struct Builder {
			unsigned long _runtimeMs;

			explicit Builder(const unsigned long runtimeMs) : _runtimeMs(runtimeMs) {}

			ConcreteBuilder &runtimeMs(unsigned long v) {
				this->_runtimeMs = v;
				return static_cast<ConcreteBuilder &>(*this);
			}
		};

		protected:
			explicit TimedEffect(
				unsigned long runtimeMs
			);
	};


	struct RandomTimedEffect : TimedEffect {
		unsigned long minRuntimeMs;
		unsigned long maxRuntimeMs;
		SamplingFunction samplingFunction;


		void sampleRuntime();

		void resetTimer() override;

		/**
		 * Standalone mixin builder (see TimedEffect::Builder for why it does not derive from LedPipelineStage::Builder).
		 * Templated on the leaf builder type (CRTP) so its setters return the leaf type and chain cleanly.
		 */
		template<typename ConcreteBuilder> struct Builder {
			unsigned long _minRuntimeMs = 0;
			unsigned long _maxRuntimeMs;
			SamplingFunction _samplingFunction = SamplingFunction::UNIFORM;

			explicit Builder(const unsigned long maxRuntimeMs) : _maxRuntimeMs(maxRuntimeMs) {}

			ConcreteBuilder &minRuntimeMs(unsigned long v) {
				this->_minRuntimeMs = v;
				return static_cast<ConcreteBuilder &>(*this);
			}

			ConcreteBuilder &maxRuntimeMs(unsigned long v) {
				this->_maxRuntimeMs = v;
				return static_cast<ConcreteBuilder &>(*this);
			}

			ConcreteBuilder &samplingFunction(SamplingFunction v) {
				this->_samplingFunction = v;
				return static_cast<ConcreteBuilder &>(*this);
			}
		};

		protected:
			RandomTimedEffect(
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction
			);
	};
}
