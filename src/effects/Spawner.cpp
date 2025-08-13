#include "effects/Spawner.h"


namespace ledpipelines::effects {


Spawner::Spawner(const Config &config)
    : BaseLedPipelineStage(),
      factory(config.factory),
      maxChildren(config.maxChildren),
      activeChildren(),
      keepOldOnSpawn(config.keepOldOnSpawn) {}


void Spawner::calculate(float startIndex, TemporaryLedData &tempData) {
    for (auto &child: activeChildren) {
        TemporaryLedData childData = TemporaryLedData();
        child->calculate(startIndex, childData);
        tempData.merge(childData, child->blendingMode);
    }


    activeChildren.erase(
        std::remove_if(activeChildren.begin(), activeChildren.end(),
                       [](BaseLedPipelineStage *child) {
                           if (child->state == LedPipelineRunningState::DONE) {
                               delete child;
                               return true;
                           };
                           return false;
                       }),
        activeChildren.end()
    );
}


void Spawner::reset() {
    BaseLedPipelineStage::reset();
    for (auto &child: activeChildren) {
        delete child;
    }
    activeChildren.clear();
}

void Spawner::spawn() {

    if (maxChildren <= 0) return;

    if (activeChildren.size() >= maxChildren) {
        if (keepOldOnSpawn) return;

        // we should delete the oldest child, which is at index 0.
        delete activeChildren[0];
        activeChildren.erase(activeChildren.begin());
    }

    BaseLedPipelineStage *newChild = factory();
    activeChildren.push_back(newChild);
}


TimedSpawner::TimedSpawner(const Config &config)
    : Spawner({
                  .factory = config.factory,
                  .maxChildren = config.maxChildren,
                  .keepOldOnSpawn = config.keepOldOnSpawn,

              }),
      spawnTimeMs(config.spawnTimeMs) {}


void TimedSpawner::calculate(float startIndex, TemporaryLedData &tempData) {
    auto currentTimeMs = millis();
    if (currentTimeMs - lastSpawnTimeMs >= spawnTimeMs) {
        lastSpawnTimeMs = currentTimeMs;
        spawn();
    }
    Spawner::calculate(startIndex, tempData);
}


RandomTimedSpawner::RandomTimedSpawner(const Config &config)
    : TimedSpawner({
                       .factory = config.factory,
                       .maxChildren = config.maxChildren,
                       .spawnTimeMs = config.spawnTimeSamplingFunction(
                           config.minSpawnTimeMs,
                           config.maxSpawnTimeMs
                       ),
                       .keepOldOnSpawn = config.keepOldOnSpawn,
                   }),
      minSpawnTimeMs(config.minSpawnTimeMs),
      maxSpawnTimeMs(config.maxSpawnTimeMs),
      spawnTimeSamplingFunction(config.spawnTimeSamplingFunction) {}


void RandomTimedSpawner::calculate(float startIndex, TemporaryLedData &tempData) {
    auto currentTimeMs = millis();
    if (currentTimeMs - lastSpawnTimeMs >= spawnTimeMs) {
        lastSpawnTimeMs = currentTimeMs;
        spawnTimeMs = spawnTimeSamplingFunction(minSpawnTimeMs, maxSpawnTimeMs);
        spawn();
    }
    Spawner::calculate(startIndex, tempData);
}
}