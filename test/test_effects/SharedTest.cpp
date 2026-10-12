#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

// Unlike every other builder, building a Shared::Builder twice must NOT construct a new inner.
TEST(SharedTest, ReusesTheSameInstanceAcrossBuilds) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto inner = std::shared_ptr<LedPipelineStage>(SpyEffect::Builder().build());
	EXPECT_EQ(SpyEffect::totalCreated, 1);
	EXPECT_EQ(SpyEffect::liveCount, 1);

	LedPipelineStage *firstShared;
	LedPipelineStage *secondShared;
	{
		// Scoped so the builder releases its reference before the final liveCount check, leaving `inner` as sole owner.
		Shared::Builder builder(inner);

		firstShared = builder.build();
		secondShared = builder.build();
		ASSERT_NE(firstShared, nullptr);
		ASSERT_NE(secondShared, nullptr);
		EXPECT_NE(firstShared, secondShared);
		EXPECT_EQ(SpyEffect::totalCreated, 1);
		EXPECT_EQ(SpyEffect::liveCount, 1);
	}

	delete firstShared;
	EXPECT_EQ(SpyEffect::liveCount, 1);
	delete secondShared;
	EXPECT_EQ(SpyEffect::liveCount, 1);

	inner.reset();
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

TEST(SharedTest, SharedInnerRendersInBothBranchesAndFreesOnce) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto dot = std::shared_ptr<LedPipelineStage>(SolidSegment::Builder(RGBA::White, 3).build());

	auto *pipeline = ParallelLedPipeline::Builder()
						 .addStage(Shared::Builder(dot).shift(0))
						 .addStage(Shared::Builder(dot).shift(5))
						 .build();
	pipeline->reset();

	TemporaryLedData data;
	pipeline->calculate(0, data);

	for (int i = 0; i < 3; i++) EXPECT_GT(data.getOpacity(i), 0) << "pixel " << i;
	for (int i = 5; i < 8; i++) EXPECT_GT(data.getOpacity(i), 0) << "pixel " << i;
	EXPECT_EQ(data.getOpacity(4), 0);

	delete pipeline;
	dot.reset();  // no crash / double free -> the test simply completing is the check
}

TEST(SharedTest, SharedConvenienceMethodBuildsInnerOnce) {
	setUpLeds();
	SpyEffect::resetCounters();

	// .shared() builds eagerly.
	auto ball = SpyEffect::Builder().shared();
	EXPECT_EQ(SpyEffect::totalCreated, 1);
	EXPECT_EQ(SpyEffect::liveCount, 1);

	auto *pipeline = ParallelLedPipeline::Builder()
						 .addStage(ball.shift(0))
						 .addStage(ball.shift(5))
						 .build();
	EXPECT_EQ(SpyEffect::totalCreated, 1) << "shared() inner must be built once, not per branch";
	EXPECT_EQ(SpyEffect::liveCount, 1);

	pipeline->reset();
	delete pipeline;
	// `ball` still holds the shared_ptr.
	EXPECT_EQ(SpyEffect::liveCount, 1);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
