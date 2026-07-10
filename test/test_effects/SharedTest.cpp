// Tests for the effects (include/effects/*). This is the test_effects PlatformIO suite: one binary, one main() at the
// bottom, and one .cpp per effect area added alongside this one. Shared fixtures live in test/utils/test_helpers.h.

#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

// Shared wraps a shared_ptr to an already-built stage rather than a builder, so the SAME instance is reused instead of
// rebuilt. Building a Shared::Builder twice must NOT construct a new inner (unlike every other builder) - both Shared
// stages point at the one instance owned by the shared_ptr.
TEST(SharedTest, ReusesTheSameInstanceAcrossBuilds) {
	setUpLeds();
	SpyEffect::resetCounters();

	// Build one spy and hand it to a Shared builder via shared_ptr. One instance created so far.
	auto inner = std::shared_ptr<LedPipelineStage>(SpyEffect::Builder().build());
	EXPECT_EQ(SpyEffect::totalCreated, 1);
	EXPECT_EQ(SpyEffect::liveCount, 1);

	LedPipelineStage *firstShared;
	LedPipelineStage *secondShared;
	{
		// The builder holds its own reference to the shared inner; scope it so it releases that reference before the
		// final liveCount check below, leaving `inner` as the sole owner.
		Shared::Builder builder(inner);

		// Two Shared stages off the same builder. Crucially, NO new spy is constructed - both wrap the existing instance.
		firstShared = builder.build();
		secondShared = builder.build();
		ASSERT_NE(firstShared, nullptr);
		ASSERT_NE(secondShared, nullptr);
		EXPECT_NE(firstShared, secondShared);       // distinct Shared wrappers...
		EXPECT_EQ(SpyEffect::totalCreated, 1);      // ...but still exactly one inner ever created
		EXPECT_EQ(SpyEffect::liveCount, 1);
	}

	// Deleting both Shared wrappers must NOT free the shared inner (the shared_ptr `inner` still owns it). This is the
	// double-free that a raw-owning wrapper would cause; Shared must not.
	delete firstShared;
	EXPECT_EQ(SpyEffect::liveCount, 1);
	delete secondShared;
	EXPECT_EQ(SpyEffect::liveCount, 1);

	// The inner dies only when the last shared_ptr referencing it drops. With the builder out of scope, `inner` is the
	// sole remaining owner, so resetting it frees the spy.
	inner.reset();
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// A Shared inner referenced from two branches of a pipeline renders in both places, and tearing the whole tree down
// frees the inner exactly once (via the shared_ptr), not once per referencing Shared.
TEST(SharedTest, SharedInnerRendersInBothBranchesAndFreesOnce) {
	setUpLeds();
	SpyEffect::resetCounters();

	// One 3-pixel white segment, shared into two parallel branches shifted to different positions.
	auto dot = std::shared_ptr<LedPipelineStage>(SolidSegment::Builder(CRGB::White, 3).build());

	auto *pipeline = ParallelLedPipeline::Builder()
						 .addStage(Shared::Builder(dot).shift(0))
						 .addStage(Shared::Builder(dot).shift(5))
						 .build();
	pipeline->reset();

	TemporaryLedData data;
	pipeline->calculate(0, data);

	// Both shifted copies of the shared segment are lit: pixels [0,3) and [5,8).
	for (int i = 0; i < 3; i++) EXPECT_GT(data.getOpacity(i), 0) << "pixel " << i;
	for (int i = 5; i < 8; i++) EXPECT_GT(data.getOpacity(i), 0) << "pixel " << i;
	// A gap between the two copies stays dark.
	EXPECT_EQ(data.getOpacity(4), 0);

	// Tearing down the pipeline deletes both Shared wrappers but must not double-free the shared segment; it stays
	// alive under `dot` until we drop it.
	delete pipeline;
	dot.reset();  // no crash / double free -> the test simply completing is the check
}

// The .shared() convenience method builds the inner eagerly (once) and returns a Shared::Builder wrapping it. Reusing
// that one Shared::Builder in two branches must yield two Shared stages backed by the SAME single instance - proven by
// SpyEffect only ever being constructed once even though the pipeline has two Shared stages.
TEST(SharedTest, SharedConvenienceMethodBuildsInnerOnce) {
	setUpLeds();
	SpyEffect::resetCounters();

	// .shared() builds the spy right here (eager - inherent to Shared, which holds a built stage). One instance.
	auto ball = SpyEffect::Builder().shared();
	EXPECT_EQ(SpyEffect::totalCreated, 1);
	EXPECT_EQ(SpyEffect::liveCount, 1);

	// Reuse the same Shared::Builder in two branches. No new spy is constructed - both Shared stages wrap the one
	// instance the builder already holds via its shared_ptr.
	auto *pipeline = ParallelLedPipeline::Builder()
						 .addStage(ball.shift(0))
						 .addStage(ball.shift(5))
						 .build();
	EXPECT_EQ(SpyEffect::totalCreated, 1) << "shared() inner must be built once, not per branch";
	EXPECT_EQ(SpyEffect::liveCount, 1);

	pipeline->reset();
	delete pipeline;
	// The shared spy is still alive: `ball`'s Shared::Builder holds the shared_ptr.
	EXPECT_EQ(SpyEffect::liveCount, 1);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
