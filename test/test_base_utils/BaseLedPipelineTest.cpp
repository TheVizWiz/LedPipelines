// Tests for the base / utility machinery (include/BaseLedPipeline.h, LedPipelineStage, the builder factory, etc.). This
// is the test_base_utils PlatformIO suite: one binary, one main() at the bottom, one .cpp per area added alongside this
// one. Shared fixtures live in test/utils/test_helpers.h.

#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

// Smallest possible end-to-end check: the stubs compile/link and a builder produces a stage.
TEST(BuilderSmokeTest, SolidBuilderProducesNonNullStage) {
	setUpLeds();

	auto *stage = Solid::Builder(RGBA::Red).build();

	ASSERT_NE(stage, nullptr);

	delete stage;
}

// The core guarantee of the deferred-inner-build / factory refactor: building the same wrapped chain twice produces
// two fully independent trees that share nothing, so each can be owned and deleted exactly once.
TEST(BuilderFactoryTest, WrappedChainBuildsIndependentTrees) {
	setUpLeds();
	SpyEffect::resetCounters();

	// A wrapper chain held as a builder: a spy leaf wrapped in a Loop. Nothing is built yet.
	auto builder = SpyEffect::Builder().wrap(Loop::Builder());

	// First build. create() should build a fresh inner spy for this tree.
	auto *first = builder.build();
	ASSERT_NE(first, nullptr);
	EXPECT_EQ(SpyEffect::liveCount, 1);
	EXPECT_EQ(SpyEffect::totalCreated, 1);

	// Second build. If the inner were shared (the old memoized/eager-build behavior), no new spy would be created and
	// both Loops would point at the same inner - the double-free we set out to eliminate. Instead we expect a brand
	// new, independent inner spy.
	auto *second = builder.build();
	ASSERT_NE(second, nullptr);

	// Distinct roots, and a second independent inner was constructed.
	EXPECT_NE(first, second);
	EXPECT_EQ(SpyEffect::liveCount, 2);
	EXPECT_EQ(SpyEffect::totalCreated, 2);

	// Each Loop owns and deletes its own inner spy; deleting both leaves nothing alive (no leak, no shared inner).
	delete first;
	EXPECT_EQ(SpyEffect::liveCount, 1);

	delete second;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// Pipeline builders defer their children the same way wrappers defer their inner: children are stored as builders and
// rebuilt fresh on each build(). Building a pipeline builder twice must therefore produce independent pipelines whose
// children share nothing. Exercises both addStage paths - a bare leaf builder and a wrap()-chained child.
TEST(BuilderFactoryTest, PipelineBuildsIndependentChildTrees) {
	setUpLeds();
	SpyEffect::resetCounters();

	// Two children: one bare spy, one spy wrapped in a Loop. Two spies per built pipeline. Capturing the chain into a
	// local needs no std::move: the fluent setters are ref-qualified, so calling addStage on the temporary builder
	// hits the &&-qualified overload that moves the builder out by value.
	auto builder = SeriesLedPipeline::Builder()
					   .addStage(SpyEffect::Builder())
					   .addStage(SpyEffect::Builder().wrap(Loop::Builder()));

	auto *first = builder.build();
	ASSERT_NE(first, nullptr);
	EXPECT_EQ(SpyEffect::liveCount, 2);
	EXPECT_EQ(SpyEffect::totalCreated, 2);

	// Second build must construct a whole new set of children rather than move the same stages out (the old
	// build-once behavior would have yielded an empty second pipeline and left totalCreated at 2).
	auto *second = builder.build();
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);
	EXPECT_EQ(SpyEffect::liveCount, 4);
	EXPECT_EQ(SpyEffect::totalCreated, 4);

	// Deleting a pipeline deletes all its children (stages are held by unique_ptr); the wrapped child's Loop also
	// deletes its inner spy. Each delete frees exactly its own tree.
	delete first;
	EXPECT_EQ(SpyEffect::liveCount, 2);

	delete second;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// The motivating ergonomic case: capture a builder, build it, add another stage, build again - to get two pipelines
// sharing a common prefix. Verifies (a) capturing a temporary chain into a local needs no std::move, (b) the captured
// lvalue builder keeps chaining in place via the &-qualified addStage, and (c) each build() is an independent factory
// so the first pipeline is unaffected by stages added afterward.
TEST(BuilderFactoryTest, SharedPrefixTwoPipelines) {
	setUpLeds();
	SpyEffect::resetCounters();

	// Captured from a temporary chain - no std::move needed (&&-qualified addStage moves it out).
	auto shared = ParallelLedPipeline::Builder().addStage(SpyEffect::Builder());

	auto *onePipeline = shared.build();  // one effect
	EXPECT_EQ(SpyEffect::liveCount, 1);

	// shared is now a named lvalue; addStage hits the &-qualified overload and mutates in place.
	shared.addStage(SpyEffect::Builder());
	auto *twoPipeline = shared.build();  // two effects, freshly built

	// The first pipeline still has exactly its original one stage; the second built its own two. 1 + 2 = 3 live.
	EXPECT_NE(onePipeline, twoPipeline);
	EXPECT_EQ(SpyEffect::liveCount, 3);
	EXPECT_EQ(SpyEffect::totalCreated, 3);

	delete onePipeline;
	EXPECT_EQ(SpyEffect::liveCount, 2);  // only the first pipeline's single stage freed

	delete twoPipeline;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// A plain builder is a reusable recipe: wrapping the SAME lvalue builder into two different parents copies it (deep
// clone), so each use produces its own independent sub-tree and the original stays valid. This is the ergonomic the
// copyable-builder change enables - no std::move, no crash, no shared state.
TEST(BuilderReuseTest, WrappingAnLvalueBuilderTwiceGivesIndependentTrees) {
	setUpLeds();
	SpyEffect::resetCounters();

	// `inner` is a reusable chain: a spy wrapped in a Loop. Nothing built yet.
	auto inner = SpyEffect::Builder().wrap(Loop::Builder());

	// Wrap the SAME `inner` into two different parents. Each wrap() copies `inner` (lvalue overload), so `inner` stays
	// usable for the second wrap.
	auto up = inner.wrap(Loop::Builder());
	auto down = inner.wrap(Loop::Builder());

	// Nothing is built during wrapping, so no spies exist yet.
	EXPECT_EQ(SpyEffect::totalCreated, 0);

	auto *upTree = up.build();
	auto *downTree = down.build();
	ASSERT_NE(upTree, nullptr);
	ASSERT_NE(downTree, nullptr);
	EXPECT_NE(upTree, downTree);

	// Two fully independent trees: each built its own spy. If wrap() had shared/moved the inner, the second tree would
	// be empty (or the two would share one spy).
	EXPECT_EQ(SpyEffect::totalCreated, 2);
	EXPECT_EQ(SpyEffect::liveCount, 2);

	// And the original `inner` is still intact - build it a third time for its own independent spy.
	auto *thirdTree = inner.build();
	EXPECT_EQ(SpyEffect::totalCreated, 3);
	EXPECT_EQ(SpyEffect::liveCount, 3);

	// Each tree owns exactly its own spy; deleting frees one at a time, no double-free.
	delete upTree;
	EXPECT_EQ(SpyEffect::liveCount, 2);
	delete downTree;
	EXPECT_EQ(SpyEffect::liveCount, 1);
	delete thirdTree;
	EXPECT_EQ(SpyEffect::liveCount, 0);
}

// addStage() is symmetric with wrap(): adding the SAME lvalue builder as a child of two different pipelines copies it,
// so each pipeline gets its own independent child and the original builder stays reusable.
TEST(BuilderReuseTest, AddingAnLvalueBuilderToTwoPipelinesGivesIndependentChildren) {
	setUpLeds();
	SpyEffect::resetCounters();

	// A reusable child recipe. Nothing built yet.
	auto child = SpyEffect::Builder().wrap(Loop::Builder());

	// Add the SAME child to two separate pipelines. Each addStage copies it (lvalue), leaving `child` intact.
	auto *p = ParallelLedPipeline::Builder().addStage(child).build();
	auto *q = ParallelLedPipeline::Builder().addStage(child).build();
	ASSERT_NE(p, nullptr);
	ASSERT_NE(q, nullptr);

	// Two independent children built - if addStage had moved `child`, q would be empty and totalCreated would be 1.
	EXPECT_EQ(SpyEffect::totalCreated, 2);
	EXPECT_EQ(SpyEffect::liveCount, 2);

	// `child` is still intact: build it once more standalone for a third independent spy.
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
