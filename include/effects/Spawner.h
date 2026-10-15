#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Spawner : LedPipelineStage {
		using EffectSpawnerFactory = std::function<LedPipelineStage*()>;

		std::vector<LedPipelineStage*> activeChildren;
		EffectSpawnerFactory factory;
		uint16_t maxChildren;
		bool keepOldOnSpawn;

		~Spawner() override;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		void spawn();

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<Spawner, Builder> {
			EffectSpawnerFactory _factory;
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			explicit Builder(EffectSpawnerFactory factory) : _factory(std::move(factory)) {};

			Spawner* build() override {
				return new Spawner(_factory, _maxChildren.get(), _keepOldOnSpawn.get(), _blendingMode);
			}
		};

	protected:
		Spawner(
			EffectSpawnerFactory factory,
			uint16_t maxChildren,
			bool keepOldOnSpawn,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};


	struct TimedSpawner : public Spawner {
		// Re-sampled after every spawn, so a RandomRange gives each gap a fresh length.
		Value<unsigned long> spawnTimeMs;

		unsigned long lastSpawnTimeMs = 0;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<TimedSpawner, Builder> {
			EffectSpawnerFactory _factory;
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			BUILDER_FIELD(unsigned long, spawnTimeMs);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			Builder(EffectSpawnerFactory factory, Value<unsigned long> spawnTimeMs)
				: _factory(std::move(factory)), _spawnTimeMs(std::move(spawnTimeMs)) {};

			TimedSpawner* build() override {
				return new TimedSpawner(_factory, _maxChildren.get(), _keepOldOnSpawn.get(), _spawnTimeMs, _blendingMode);
			}
		};

	protected:
		TimedSpawner(
			EffectSpawnerFactory factory,
			uint16_t maxChildren,
			bool keepOldOnSpawn,
			Value<unsigned long> spawnTimeMs,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
}
