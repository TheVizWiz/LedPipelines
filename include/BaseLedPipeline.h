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
		template<typename T> struct Builder {
			virtual ~Builder() = default;

			virtual T *build() = 0;
		};

		LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;


		virtual void calculate(float startIndex, TemporaryLedData &tempData) = 0;

		virtual void run();

		virtual void reset();

		explicit LedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

		virtual ~LedPipelineStage();

		template<class T, typename... Args> fl::enable_if_t<std::is_base_of_v<effects::WrapperEffect, T>, T *> wrap(
			Args &&... args) {
			return new T(this, std::forward<Args>(args)...);
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

		BlendingMode blendingMode;
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
