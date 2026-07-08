#pragma once

#include <utility>

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

		struct Builder : LedPipelineStage::Builder<Spawner, Builder> {
			BUILDER_FIELD(EffectSpawnerFactory, factory);
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			explicit Builder(EffectSpawnerFactory factory) : _factory(std::move(factory)) {};

			Spawner* build() override {
				return new Spawner(_factory, _maxChildren, _keepOldOnSpawn, _blendingMode);
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
		unsigned long spawnTimeMs;

		unsigned long lastSpawnTimeMs = 0;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : LedPipelineStage::Builder<TimedSpawner, Builder> {
			BUILDER_FIELD(EffectSpawnerFactory, factory);
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			BUILDER_FIELD(unsigned long, spawnTimeMs);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			Builder(EffectSpawnerFactory factory, const unsigned long spawnTimeMs)
				: _factory(std::move(factory)), _spawnTimeMs(spawnTimeMs) {};

			TimedSpawner* build() override {
				return new TimedSpawner(_factory, _maxChildren, _keepOldOnSpawn, _spawnTimeMs, _blendingMode);
			}
		};

	protected:
		TimedSpawner(
			EffectSpawnerFactory factory,
			uint16_t maxChildren,
			bool keepOldOnSpawn,
			unsigned long spawnTimeMs,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};


	struct RandomTimedSpawner : public TimedSpawner {
		unsigned long minSpawnTimeMs;
		unsigned long maxSpawnTimeMs;
		SamplingFunction spawnTimeSamplingFunction;

		void calculate(float startIndex, TemporaryLedData& tempData) override;

		struct Builder : LedPipelineStage::Builder<RandomTimedSpawner, Builder> {
			BUILDER_FIELD(EffectSpawnerFactory, factory);
			BUILDER_FIELD_DEFAULT(uint16_t, maxChildren, 10);
			BUILDER_FIELD_DEFAULT(unsigned long, minSpawnTimeMs, 0);
			BUILDER_FIELD(unsigned long, maxSpawnTimeMs);
			BUILDER_FIELD_DEFAULT(SamplingFunction, spawnTimeSamplingFunction, SamplingFunction::UNIFORM);
			BUILDER_FIELD_DEFAULT(bool, keepOldOnSpawn, true);

			Builder(EffectSpawnerFactory factory, const unsigned long maxSpawnTimeMs)
				: _factory(std::move(factory)), _maxSpawnTimeMs(maxSpawnTimeMs) {};

			RandomTimedSpawner* build() override {
				return new RandomTimedSpawner(
					_factory,
					_maxChildren,
					_keepOldOnSpawn,
					_minSpawnTimeMs,
					_maxSpawnTimeMs,
					_spawnTimeSamplingFunction,
					_blendingMode
				);
			}
		};

	protected:
		RandomTimedSpawner(
			EffectSpawnerFactory factory,
			uint16_t maxChildren,
			bool keepOldOnSpawn,
			unsigned long minSpawnTimeMs,
			unsigned long maxSpawnTimeMs,
			SamplingFunction spawnTimeSamplingFunction,
			BlendingMode blendingMode = BlendingMode::NORMAL
		);
	};
} // namespace ledpipelines::effects
