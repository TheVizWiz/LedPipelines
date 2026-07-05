#include <gtest/gtest.h>

#include "LedPipelines.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

namespace {
	// Backing buffer for a single fake strip. Registered with the stub FastLED controller so TemporaryLedData::size
	// is non-zero once initialize() runs.
	constexpr int kLedCount = 10;
	CRGB gLeds[kLedCount];

	// Register one strip and initialize LedPipelines' static LED bookkeeping. Resets the fake controller first so
	// strips don't accumulate across tests.
	void setUpLeds() {
		FastLED.reset();
		FastLED.addLeds(gLeds, kLedCount);
		TemporaryLedData::initialize();
	}

	// A leaf effect that tracks how many instances currently exist (liveCount) and how many have ever been
	// constructed (totalCreated). Used to observe, from outside, whether the builder rebuilds a fresh inner effect on
	// each build() and whether wrappers correctly own/delete their inner. calculate() is a no-op.
	struct SpyEffect : LedPipelineStage {
		static int liveCount;
		static int totalCreated;

		static void resetCounters() {
			liveCount = 0;
			totalCreated = 0;
		}

		SpyEffect() {
			liveCount++;
			totalCreated++;
		}

		~SpyEffect() override { liveCount--; }

		void calculate(float, TemporaryLedData &) override {}

		struct Builder : LedPipelineStage::Builder<SpyEffect, Builder> {
			SpyEffect *build() override { return new SpyEffect(); }
		};
	};

	int SpyEffect::liveCount = 0;
	int SpyEffect::totalCreated = 0;
}

// Smallest possible end-to-end check: the stubs compile/link and a builder produces a stage.
TEST(BuilderSmokeTest, SolidBuilderProducesNonNullStage) {
	setUpLeds();

	auto *stage = Solid::Builder(CRGB::Red).build();

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

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
