#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

TEST(BuilderSmokeTest, SolidBuilderProducesNonNullStage) {
	setUpLeds();

	auto *stage = Solid::Builder(RGBA::Red).build();

	ASSERT_NE(stage, nullptr);

	delete stage;
}

TEST(BuilderFactoryTest, WrappedChainBuildsIndependentTrees) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto builder = SpyEffect::Builder().wrap(Loop::Builder());

	auto *first = builder.build();
	ASSERT_NE(first, nullptr);
	EXPECT_EQ(SpyEffect::liveCount, 1);
	EXPECT_EQ(SpyEffect::totalCreated, 1);

	auto *second = builder.build();
	ASSERT_NE(second, nullptr);

	EXPECT_NE(first, second);
	EXPECT_EQ(SpyEffect::liveCount, 2);
	EXPECT_EQ(SpyEffect::totalCreated, 2);

	delete first;
	EXPECT_EQ(SpyEffect::liveCount, 1);

	delete second;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// Exercises both addStage paths - a bare leaf builder and a wrap()-chained child.
TEST(BuilderFactoryTest, PipelineBuildsIndependentChildTrees) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto builder = SeriesLedPipeline::Builder()
					   .addStage(SpyEffect::Builder())
					   .addStage(SpyEffect::Builder().wrap(Loop::Builder()));

	auto *first = builder.build();
	ASSERT_NE(first, nullptr);
	EXPECT_EQ(SpyEffect::liveCount, 2);
	EXPECT_EQ(SpyEffect::totalCreated, 2);

	auto *second = builder.build();
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);
	EXPECT_EQ(SpyEffect::liveCount, 4);
	EXPECT_EQ(SpyEffect::totalCreated, 4);

	delete first;
	EXPECT_EQ(SpyEffect::liveCount, 2);

	delete second;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// Capture a builder, build it, add another stage, build again: the first pipeline must be unaffected by stages added
// afterward.
TEST(BuilderFactoryTest, SharedPrefixTwoPipelines) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto shared = ParallelLedPipeline::Builder().addStage(SpyEffect::Builder());

	auto *onePipeline = shared.build();
	EXPECT_EQ(SpyEffect::liveCount, 1);

	shared.addStage(SpyEffect::Builder());
	auto *twoPipeline = shared.build();

	EXPECT_NE(onePipeline, twoPipeline);
	EXPECT_EQ(SpyEffect::liveCount, 3);
	EXPECT_EQ(SpyEffect::totalCreated, 3);

	delete onePipeline;
	EXPECT_EQ(SpyEffect::liveCount, 2);

	delete twoPipeline;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// Wrapping an lvalue builder copies it, so the original stays usable.
TEST(BuilderReuseTest, WrappingAnLvalueBuilderTwiceGivesIndependentTrees) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto inner = SpyEffect::Builder().wrap(Loop::Builder());

	auto up = inner.wrap(Loop::Builder());
	auto down = inner.wrap(Loop::Builder());

	EXPECT_EQ(SpyEffect::totalCreated, 0);

	auto *upTree = up.build();
	auto *downTree = down.build();
	ASSERT_NE(upTree, nullptr);
	ASSERT_NE(downTree, nullptr);
	EXPECT_NE(upTree, downTree);

	EXPECT_EQ(SpyEffect::totalCreated, 2);
	EXPECT_EQ(SpyEffect::liveCount, 2);

	auto *thirdTree = inner.build();
	EXPECT_EQ(SpyEffect::totalCreated, 3);
	EXPECT_EQ(SpyEffect::liveCount, 3);

	delete upTree;
	EXPECT_EQ(SpyEffect::liveCount, 2);
	delete downTree;
	EXPECT_EQ(SpyEffect::liveCount, 1);
	delete thirdTree;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

TEST(BuilderReuseTest, AddingAnLvalueBuilderToTwoPipelinesGivesIndependentChildren) {
	setUpLeds();
	SpyEffect::resetCounters();

	auto child = SpyEffect::Builder().wrap(Loop::Builder());

	auto *p = ParallelLedPipeline::Builder().addStage(child).build();
	auto *q = ParallelLedPipeline::Builder().addStage(child).build();
	ASSERT_NE(p, nullptr);
	ASSERT_NE(q, nullptr);

	EXPECT_EQ(SpyEffect::totalCreated, 2);
	EXPECT_EQ(SpyEffect::liveCount, 2);

	auto *r = child.build();
	EXPECT_EQ(SpyEffect::totalCreated, 3);
	EXPECT_EQ(SpyEffect::liveCount, 3);

	delete p;
	EXPECT_EQ(SpyEffect::liveCount, 2);
	delete q;
	EXPECT_EQ(SpyEffect::liveCount, 1);
	delete r;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
