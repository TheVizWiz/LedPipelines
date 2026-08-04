#pragma once

#include <functional>  // std::function (the factory type)
#include <utility>
#include <vector>  // activeChildren

#include "BaseEffect.h"

namespace ledpipelines::effects {
	struct Spawner : LedPipelineStage {
		using EffectSpawnerFactory = std::function<LedPipelineStage*()>;

		std::vector<LedPipelineStage*> activeChildren;
		EffectSpawnerFactory factory;
		uint16_t maxChildren;
		bool keepOldOnSpawn;


		void calculate(float startIndex, TemporaryLedData& tempData) override;

		void reset() override;

		void spawn();

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<Spawner, Builder> {
			// factory is a callable, not a randomizable value, so it is a plain field (hand-rolled, not via the
			// Value-wrapping BUILDER_FIELD macro). The other fields are Value<T> recipes; build() reads their .value.
			EffectSpawnerFactory _factory;
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			explicit Builder(EffectSpawnerFactory factory) : _factory(std::move(factory)) {};

			Spawner* build() override {
				return new Spawner(_factory, _maxChildren.value, _keepOldOnSpawn.value, _blendingMode);
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
		// The spawn interval: a Value<unsigned long> caching its own draw (see Value). A constant keeps a fixed cadence;
		// a RandomRange gives irregular spacing - this replaces the old RandomTimedSpawner. Unlike the once-per-lifecycle
		// sampling elsewhere, a spawn interval's "lifecycle" is a single interval: spawnTimeMs is re-sampled after every
		// spawn (calculate()), so each gap gets a fresh random length. calculate() waits spawnTimeMs.value between spawns.
		Value<unsigned long> spawnTimeMs;

		unsigned long lastSpawnTimeMs = 0;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool includeState) override;
#endif

		struct Builder : LedPipelineStage::Builder<TimedSpawner, Builder> {
			// factory is a plain field (see Spawner::Builder). The rest are Value<T> recipes; build() reads their .value
			// except spawnTimeMs, which is passed as a Value so the effect can re-sample it each spawn.
			EffectSpawnerFactory _factory;
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			// A plain interval converts implicitly (regular cadence), or pass a RandomRange for irregular spacing.
			BUILDER_FIELD(unsigned long, spawnTimeMs);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			Builder(EffectSpawnerFactory factory, Value<unsigned long> spawnTimeMs)
				: _factory(std::move(factory)), _spawnTimeMs(std::move(spawnTimeMs)) {};

			TimedSpawner* build() override {
				return new TimedSpawner(_factory, _maxChildren.value, _keepOldOnSpawn.value, _spawnTimeMs, _blendingMode);
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
} // namespace ledpipelines::effects
