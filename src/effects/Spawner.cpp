#include "effects/Spawner.h"

#include <algorithm>
#include <utility>


namespace ledpipelines::effects {

	Spawner::Spawner(EffectSpawnerFactory factory, uint16_t maxChildren, bool keepOldOnSpawn, BlendingMode blendingMode)
		: LedPipelineStage(blendingMode),
		  factory(std::move(factory)),
		  maxChildren(maxChildren),
		  keepOldOnSpawn(keepOldOnSpawn) {}


	Spawner::~Spawner() {
		for (auto& child : activeChildren) delete child;
	}


	void Spawner::calculate(float startIndex, TemporaryLedData& tempData) {
		// Re-arm after reset(); TimedSpawner delegates here, so this covers both. A Spawner never reaches DONE.
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
		Value<unsigned long> spawnTimeMs,
		BlendingMode blendingMode
	)
		: Spawner(std::move(factory), maxChildren, keepOldOnSpawn, blendingMode), spawnTimeMs(std::move(spawnTimeMs)) {
		this->spawnTimeMs.sample();
	}


	void TimedSpawner::calculate(float startIndex, TemporaryLedData& tempData) {
		auto currentTimeMs = millis();
		if (currentTimeMs - lastSpawnTimeMs >= spawnTimeMs.get()) {
			lastSpawnTimeMs = currentTimeMs;
			spawnTimeMs.sample();
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
		result += jsonRawField("spawnTimeMs", spawnTimeMs.describeValue());
		result += jsonField("factory", std::string("non-serializable"));
		if (includeState) {
			std::string kids = "[";
			for (size_t i = 0; i < activeChildren.size(); i++) {
				if (i > 0) kids += ",";
				kids += activeChildren[i]->toJson(includeState);
			}
			kids += "]";
			result += jsonStateObject(
				jsonStateHeader() + jsonField("lastSpawnTimeMs", (int)lastSpawnTimeMs) +
				jsonField("sampledSpawnTimeMs", (int)spawnTimeMs.value) + jsonRawField("activeChildren", kids));
		}
		result += "}";
		return result;
	}
#endif
}
