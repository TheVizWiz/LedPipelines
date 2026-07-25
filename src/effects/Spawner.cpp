#include "effects/Spawner.h"

#include <algorithm>  // std::remove_if - not guaranteed transitively on all standard libraries (e.g. MinGW)
#include <utility>


namespace ledpipelines::effects {

	Spawner::Spawner(EffectSpawnerFactory factory, uint16_t maxChildren, bool keepOldOnSpawn, BlendingMode blendingMode)
		: LedPipelineStage(blendingMode),
		  factory(std::move(factory)),
		  maxChildren(maxChildren),
		  keepOldOnSpawn(keepOldOnSpawn) {}


	void Spawner::calculate(float startIndex, TemporaryLedData& tempData) {
		// A Spawner runs indefinitely (it has no completion condition of its own), but it must report RUNNING once it
		// starts rather than sitting at NOT_STARTED. TimedSpawner/RandomTimedSpawner delegate to this method, so this
		// single guard re-arms the whole family after a reset(). It never reaches DONE - a Spawner does not finish.
		if (this->state == LedPipelineRunningState::NOT_STARTED) this->state = LedPipelineRunningState::RUNNING;

		for (auto& child : activeChildren) {
			TemporaryLedData childData = TemporaryLedData();
			child->calculate(startIndex, childData);
			tempData.merge(childData, child->blendingMode);
		}


		activeChildren.erase(
			std::remove_if(
				activeChildren.begin(),
				activeChildren.end(),
				[](LedPipelineStage* child) {
					if (child->state == LedPipelineRunningState::DONE) {
						delete child;
						return true;
					};
					return false;
				}
			),
			activeChildren.end()
		);
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


	TimedSpawner::TimedSpawner(
		EffectSpawnerFactory factory,
		uint16_t maxChildren,
		bool keepOldOnSpawn,
		unsigned long spawnTimeMs,
		BlendingMode blendingMode
	)
		: Spawner(std::move(factory), maxChildren, keepOldOnSpawn, blendingMode), spawnTimeMs(spawnTimeMs) {}


	void TimedSpawner::calculate(float startIndex, TemporaryLedData& tempData) {
		auto currentTimeMs = millis();
		if (currentTimeMs - lastSpawnTimeMs >= spawnTimeMs) {
			lastSpawnTimeMs = currentTimeMs;
			spawn();
		}
		Spawner::calculate(startIndex, tempData);
	}


	RandomTimedSpawner::RandomTimedSpawner(
		EffectSpawnerFactory factory,
		uint16_t maxChildren,
		bool keepOldOnSpawn,
		unsigned long minSpawnTimeMs,
		unsigned long maxSpawnTimeMs,
		SamplingFunction spawnTimeSamplingFunction,
		BlendingMode blendingMode
	)
		: TimedSpawner(
			  std::move(factory),
			  maxChildren,
			  keepOldOnSpawn,
			  spawnTimeSamplingFunction(minSpawnTimeMs, maxSpawnTimeMs),
			  blendingMode
		  ),
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


#ifdef LEDPIPELINES_SERIALIZATION
	std::string Spawner::toJson(bool includeState) {
		std::string result = "{";
		result += jsonHeader("Spawner");
		result += jsonField("maxChildren", (int)maxChildren);
		result += jsonField("keepOldOnSpawn", keepOldOnSpawn);
		result += jsonField("factory", std::string("non-serializable"));
		if (includeState) {
			std::string kids = "[";
			for (size_t i = 0; i < activeChildren.size(); i++) {
				if (i > 0) kids += ",";
				kids += activeChildren[i]->toJson(includeState);
			}
			kids += "]";
			result += jsonStateObject(jsonStateHeader() + jsonRawField("activeChildren", kids));
		}
		result += "}";
		return result;
	}

	std::string TimedSpawner::toJson(bool includeState) {
		std::string result = "{";
		result += jsonHeader("TimedSpawner");
		result += jsonField("maxChildren", (int)maxChildren);
		result += jsonField("keepOldOnSpawn", keepOldOnSpawn);
		result += jsonField("spawnTimeMs", (int)spawnTimeMs);
		result += jsonField("factory", std::string("non-serializable"));
		if (includeState) {
			std::string kids = "[";
			for (size_t i = 0; i < activeChildren.size(); i++) {
				if (i > 0) kids += ",";
				kids += activeChildren[i]->toJson(includeState);
			}
			kids += "]";
			result += jsonStateObject(jsonStateHeader() + jsonField("lastSpawnTimeMs", (int)lastSpawnTimeMs) + jsonRawField("activeChildren", kids));
		}
		result += "}";
		return result;
	}

	std::string RandomTimedSpawner::toJson(bool includeState) {
		std::string result = "{";
		result += jsonHeader("RandomTimedSpawner");
		result += jsonField("maxChildren", (int)maxChildren);
		result += jsonField("keepOldOnSpawn", keepOldOnSpawn);
		result += jsonField("minSpawnTimeMs", (int)minSpawnTimeMs);
		result += jsonField("maxSpawnTimeMs", (int)maxSpawnTimeMs);
		result += jsonField("spawnTimeSamplingFunction", spawnTimeSamplingFunction);
		result += jsonField("factory", std::string("non-serializable"));
		if (includeState) {
			std::string kids = "[";
			for (size_t i = 0; i < activeChildren.size(); i++) {
				if (i > 0) kids += ",";
				kids += activeChildren[i]->toJson(includeState);
			}
			kids += "]";
			result += jsonStateObject(jsonStateHeader() + jsonField("lastSpawnTimeMs", (int)lastSpawnTimeMs) + jsonRawField("activeChildren", kids));
		}
		result += "}";
		return result;
	}
#endif
} // namespace ledpipelines::effects
