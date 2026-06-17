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

			template<typename T> class Builder : public LedPipelineStage::Builder<T> {
				public:
					BUILDER_FIELD(LedPipelineStage *, stage);

					template<typename U> Builder &setStage(
						LedPipelineStage::Builder<U> &stage
					) {
						this->_stage = stage;
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

		struct Builder : LedPipelineStage::Builder<TimedEffect> {
			BUILDER_FIELD(unsigned long, runtimeMs);

			explicit Builder(const unsigned long runtimeMs) : _runtimeMs(runtimeMs) {}
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

		struct Builder : LedPipelineStage::Builder<RandomTimedEffect> {
			BUILDER_FIELD_DEFAULT(unsigned long, minRuntimeMs, 0);
			BUILDER_FIELD(unsigned long, maxRuntimeMs);
			BUILDER_FIELD_DEFAULT(SamplingFunction, samplingFunction, SamplingFunction::UNIFORM);

			explicit Builder(const unsigned long maxRuntimeMs) : _maxRuntimeMs(maxRuntimeMs) {}
		};

		protected:
			RandomTimedEffect(
				unsigned long minRuntimeMs,
				unsigned long maxRuntimeMs,
				SamplingFunction samplingFunction
			);
	};
}
