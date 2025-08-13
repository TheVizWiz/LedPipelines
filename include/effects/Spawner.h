#pragma once

#include "BaseEffect.h"

namespace ledpipelines::effects {

struct Spawner : public BaseLedPipelineStage {

    using EffectSpawnerFactory = std::function<BaseLedPipelineStage *()>;


    struct Config {
        RequiredField<EffectSpawnerFactory> factory;
        uint16_t maxChildren = 10;
        /**
         * when set to true, if spawn is called when number of children
         * is already maxed out, then the new spawned effect will be
         * ignored. otherwise, the oldest child will be discarded,
         * and the newest child will be used instead.
         */
        bool keepOldOnSpawn = true;
    };

    std::vector<BaseLedPipelineStage *> activeChildren;
    EffectSpawnerFactory factory;
    uint16_t maxChildren;
    bool keepOldOnSpawn;

    Spawner(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData);

    void reset() override;

    void spawn();
};


struct TimedSpawner : public Spawner {

    struct Config {
        RequiredField<EffectSpawnerFactory> factory;
        uint16_t maxChildren = 10;
        RequiredField<unsigned long> spawnTimeMs;
        /**
         * when set to true, if spawn is called when number of children
         * is already maxed out, then the new spawned effect will be
         * ignored. otherwise, the oldest child will be discarded,
         * and the newest child will be used instead.
         */
        bool keepOldOnSpawn = true;
    };

    TimedSpawner(const Config &config);

    unsigned long spawnTimeMs;

    unsigned long lastSpawnTimeMs = 0;

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};


struct RandomTimedSpawner : public TimedSpawner {

    struct Config {
        EffectSpawnerFactory factory;
        uint16_t maxChildren = 10;
        unsigned long minSpawnTimeMs = 0;
        RequiredField<unsigned long> maxSpawnTimeMs;
        SamplingFunction spawnTimeSamplingFunction = SamplingFunction::UNIFORM;
        /**
         * when set to true, if spawn is called when number of children
         * is already maxed out, then the new spawned effect will be
         * ignored. otherwise, the oldest child will be discarded,
         * and the newest child will be used instead.
         */
        bool keepOldOnSpawn = true;
    };

    unsigned long minSpawnTimeMs;
    unsigned long maxSpawnTimeMs;
    SamplingFunction spawnTimeSamplingFunction;

    RandomTimedSpawner(const Config &config);

    void calculate(float startIndex, TemporaryLedData &tempData) override;
};

}
