#include "effects/Spawner.h"

#include <utility>


namespace ledpipelines::effects {

	Spawner::Spawner(EffectSpawnerFactory factory, uint16_t maxChildren, bool keepOldOnSpawn)
		: factory(std::move(factory)), maxChildren(maxChildren), keepOldOnSpawn(keepOldOnSpawn) {}


	void Spawner::calculate(float startIndex, TemporaryLedData& tempData) {
		for (auto& child : activeChildren) {
			TemporaryLedData childData = TemporaryLedData();
			child->calculate(startIndex, childData);
			tempData.merge(childData, BlendingMode::NORMAL);
		}


		activeChildren.erase(std::remove_if(activeChildren.begin(), activeChildren.end(),
											[](LedPipelineStage* child) {
												if (child->state == LedPipelineRunningState::DONE) {
													delete child;
													return true;
												};
												return false;
											}),
							 activeChildren.end());
	}


	void Spawner::reset() {
		LedPipelineStage::reset();
		for (auto& child : activeChildren) {
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

		LedPipelineStage* newChild = factory();
		activeChildren.push_back(newChild);
	}


	TimedSpawner::TimedSpawner(EffectSpawnerFactory factory, uint16_t maxChildren, bool keepOldOnSpawn,
							   unsigned long spawnTimeMs)
		: Spawner(std::move(factory), maxChildren, keepOldOnSpawn), spawnTimeMs(spawnTimeMs) {}


	void TimedSpawner::calculate(float startIndex, TemporaryLedData& tempData) {
		auto currentTimeMs = millis();
		if (currentTimeMs - lastSpawnTimeMs >= spawnTimeMs) {
			lastSpawnTimeMs = currentTimeMs;
			spawn();
		}
		Spawner::calculate(startIndex, tempData);
	}


	RandomTimedSpawner::RandomTimedSpawner(EffectSpawnerFactory factory, uint16_t maxChildren, bool keepOldOnSpawn,
										   unsigned long minSpawnTimeMs, unsigned long maxSpawnTimeMs,
										   SamplingFunction spawnTimeSamplingFunction)
		: TimedSpawner(std::move(factory), maxChildren, keepOldOnSpawn,
					   spawnTimeSamplingFunction(minSpawnTimeMs, maxSpawnTimeMs)),
		  minSpawnTimeMs(minSpawnTimeMs),
		  maxSpawnTimeMs(maxSpawnTimeMs),
		  spawnTimeSamplingFunction(spawnTimeSamplingFunction) {}


	void RandomTimedSpawner::calculate(float startIndex, TemporaryLedData& tempData) {
		auto currentTimeMs = millis();
		if (currentTimeMs - lastSpawnTimeMs >= spawnTimeMs) {
			lastSpawnTimeMs = currentTimeMs;
			spawnTimeMs = spawnTimeSamplingFunction(minSpawnTimeMs, maxSpawnTimeMs);
			spawn();
		}
		Spawner::calculate(startIndex, tempData);
	}
} // namespace ledpipelines::effects
