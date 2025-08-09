#pragma once

#include "FastLED.h"
#include "LedPipelineUtils.h"

namespace ledpipelines {

enum class LedPipelineRunningState {
    NOT_STARTED,
    RUNNING,
    DONE
};

namespace effects {
class WrapperEffect;
}

class BaseLedPipelineStage {
public:

    LedPipelineRunningState state = LedPipelineRunningState::NOT_STARTED;

    BlendingMode blendingMode;

    BaseLedPipelineStage *nextStage = nullptr;

    virtual void calculate(float startIndex, TemporaryLedData &tempData) = 0;

    virtual void run();

    virtual void reset();

    explicit BaseLedPipelineStage(BlendingMode blendingMode = BlendingMode::NORMAL);

    virtual ~BaseLedPipelineStage();

    template<class T, typename ... Args>
    fl::enable_if_t<std::is_base_of<effects::WrapperEffect, T>::value, T *> wrap(Args &&... args) {
        return new T(this, std::forward<Args>(args)...);
    }


private:
    u_int64_t lastUpdateTimeMicros;
};


/**
 * Base abstract class for defining LED pipelines - the construct that can be run
 * and show effects on the
 */
class LedPipeline : public BaseLedPipelineStage {

public:

    virtual LedPipeline *addStage(BaseLedPipelineStage *stage);

    void reset() override;

    explicit LedPipeline(BlendingMode mode);

    ~LedPipeline() override;

protected:
    /**
     * linked list of first and last stages. used when adding stages and state them.
     */
    BaseLedPipelineStage *firstStage = nullptr;
    BaseLedPipelineStage *lastStage = nullptr;

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

    SeriesLedPipeline(BlendingMode mode = BlendingMode::NORMAL);

    void calculate(float startIndex, TemporaryLedData &tempData) override;

    void reset() override;

private:
    BaseLedPipelineStage *currentStage = nullptr;
};


}