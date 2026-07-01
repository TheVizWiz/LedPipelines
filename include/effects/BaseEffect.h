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
					BUILDER_FIELD(LedPipelineStage *, stage);

					template<typename U, typename V> Builder &setStage(
						LedPipelineStage::Builder<U, V> &stage
					) {
						this->_stage = stage.build();
						return *this;
					}

					T *build() override = 0;
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
