#include <gtest/gtest.h>

#include <type_traits>

#include "../utils/test_helpers.h"

namespace {
	template <typename B, typename H, typename = void>
	struct CanBind : std::false_type {};

	template <typename B, typename H>
	struct CanBind<B, H, std::void_t<decltype(std::declval<B &>().bind(std::declval<const H &>()))>> : std::true_type {};

	Spawner::Builder spyFactorySpawner() {
		return Spawner::Builder([]() -> LedPipelineStage * { return SpyEffect::Builder().build(); });
	}
}

static_assert(CanBind<Spawner::Builder, Handle<Spawner>>::value);
static_assert(CanBind<TimedSpawner::Builder, Handle<Spawner>>::value, "a handle to a base type binds to derived products");
static_assert(!CanBind<Spawner::Builder, Handle<Toggle>>::value, "mismatched handle types are rejected");

TEST(HandleTest, ReachesAStageNestedInsideWrappers) {
	setUpLeds();

	Handle<Spawner> spawner;
	auto *pipeline = spyFactorySpawner().bind(spawner).wrap(Shift::Builder(1)).loop().build();

	ASSERT_EQ(spawner.size(), 1u);
	spawner->spawn();
	EXPECT_EQ(spawner->activeChildren.size(), 1u);

	delete pipeline;
}

TEST(HandleTest, BindsWhateverTheBuilderAtThatPointProduces) {
	setUpLeds();

	Handle<Loop> loop;
	auto *pipeline = SpyEffect::Builder().wrap(Loop::Builder()).bind(loop).wrap(Shift::Builder(1)).build();

	ASSERT_TRUE(loop);
	EXPECT_EQ(static_cast<WrapperEffect *>(pipeline)->innerStage(), loop.get());

	delete pipeline;
}

TEST(HandleTest, EveryBuildRegistersAndCallAllReachesEach) {
	setUpLeds();

	Handle<Spawner> spawners;
	auto builder = spyFactorySpawner().bind(spawners);
	auto *first = builder.build();
	auto copy = builder;
	auto *second = copy.build();

	ASSERT_EQ(spawners.size(), 2u);
	spawners.callAll(&Spawner::spawn);
	EXPECT_EQ(first->activeChildren.size(), 1u);
	EXPECT_EQ(second->activeChildren.size(), 1u);

	delete first;
	delete second;
}

TEST(HandleTest, DeletedStagesUnregister) {
	setUpLeds();

	Handle<Toggle> toggle;
	auto *pipeline = SpyEffect::Builder().wrap(Toggle::Builder()).bind(toggle).loop().build();
	ASSERT_TRUE(toggle);

	delete pipeline;
	EXPECT_FALSE(toggle);
	EXPECT_EQ(toggle.get(), nullptr);
}

TEST(HandleTest, StageOutlivingItsHandleIsSafe) {
	setUpLeds();

	LedPipelineStage *pipeline;
	{
		Handle<Toggle> toggle;
		pipeline = SpyEffect::Builder().wrap(Toggle::Builder()).bind(toggle).build();
	}
	delete pipeline;
}

TEST(HandleTest, ForEachToleratesStagesBuiltDuringIteration) {
	setUpLeds();

	Handle<Toggle> toggles;
	auto *spawner = Spawner::Builder([toggles]() -> LedPipelineStage * {
		return SpyEffect::Builder().wrap(Toggle::Builder()).bind(toggles).build();
	}).build();

	spawner->spawn();
	ASSERT_EQ(toggles.size(), 1u);

	int visited = 0;
	toggles.forEach([&](Toggle *) {
		visited++;
		spawner->spawn();
	});
	EXPECT_EQ(visited, 1);
	EXPECT_EQ(toggles.size(), 2u);

	delete spawner;
	EXPECT_EQ(toggles.size(), 0u);
}
