// Tests for the terminateOnComplete timing flag (include/effects/BaseEffect.h). Part of the test_effects suite: no
// main() here - SharedTest.cpp owns the suite's main(), and PlatformIO links every .cpp in the folder into one binary.
// Shared fixtures live in test/utils/test_helpers.h.
//
// terminateOnComplete flips an effect that normally defers its DONE to its wrapped inner (FadeIn, Moving) into one that
// finishes the instant its own timer/ramp/move completes. Effects that already terminate on their own timer are
// unaffected. These tests drive the fake clock deterministically via resetTestClock()/advanceTestClock().

#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

namespace {
	// Render one frame: run the stage's calculate() into a throwaway buffer. Wrapping this keeps each timing step to one
	// line so the clock advances read clearly.
	void tick(LedPipelineStage *stage) {
		TemporaryLedData data;
		stage->calculate(0, data);
	}
}  // namespace

// Baseline: without the flag, FadeIn is a pass-through once its ramp completes and defers DONE to its inner. Wrapping a
// SpyEffect (which never reports DONE) means FadeIn must stay RUNNING forever after the ramp, never finishing on its
// own. This is the behavior terminateOnComplete opts OUT of.
TEST(TerminateOnCompleteTest, FadeInDefaultDefersToInnerAndNeverFinishesOnItsOwn) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	auto *fade = SpyEffect::Builder().wrap(FadeIn::Builder(1000)).build();

	// First frame stamps startTimeMs at t=0 and begins the ramp.
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING);

	// Advance well past the 1000ms ramp. The ramp is complete, but with the flag unset FadeIn keeps passing the inner
	// through and never finishes on its own (the Spy inner never goes DONE).
	advanceTestClock(5000);
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING) << "default FadeIn must defer to its inner, not self-finish";

	delete fade;
}

// With terminateOnComplete(true), FadeIn reports DONE the instant its ramp completes, regardless of the inner. Before
// the ramp finishes it is still RUNNING; once elapsed >= runtime it flips to DONE.
TEST(TerminateOnCompleteTest, FadeInTerminatesWhenRampCompletes) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	auto *fade = SpyEffect::Builder().wrap(FadeIn::Builder(1000).terminateOnComplete(true)).build();

	// Partway through the ramp: still fading in, not done.
	tick(fade);  // stamps start at t=0
	advanceTestClock(500);
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING) << "should still be ramping mid-fade";

	// Reach the end of the ramp: now it must finish on its own, not wait for the (never-finishing) Spy inner.
	advanceTestClock(500);  // total elapsed = 1000 == runtime
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::DONE) << "terminateOnComplete FadeIn must finish at ramp end";

	delete fade;
}

// RandomFadeIn honors the flag too. This also guards the applyTiming() wiring: RandomFadeIn::Builder::build() must run
// the product through applyTiming(), or terminateOnComplete (and delayMs) would silently never reach the effect. With
// min == max the sampled runtime is deterministic (1000ms), so we can assert an exact boundary.
TEST(TerminateOnCompleteTest, RandomFadeInTerminatesWhenSampledRampCompletes) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	// minRuntimeMs defaults to 0; pin both ends to 1000 so the uniform sample is exactly 1000ms.
	auto *fade =
		SpyEffect::Builder().wrap(RandomFadeIn::Builder(1000).minRuntimeMs(1000).terminateOnComplete(true)).build();

	tick(fade);  // stamps start at t=0 and samples runtime (1000)
	advanceTestClock(999);
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING) << "one ms before the sampled ramp end";

	advanceTestClock(1);  // total elapsed = 1000 == sampled runtime
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::DONE)
		<< "terminateOnComplete must propagate through applyTiming to RandomFadeIn";

	delete fade;
}

// Moving normally holds at endPosition and defers DONE to its inner (its Model-B behavior). terminateOnComplete flips
// that: it finishes as soon as the move completes. We verify both the default hold and the opt-out on the same setup.
TEST(TerminateOnCompleteTest, MovingDefaultHoldsButTerminatesWithFlag) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	// Default: holds at endPosition, defers to the never-finishing Spy inner -> stays RUNNING past the move.
	auto *held = SpyEffect::Builder().wrap(Moving::Builder(1000).startPosition(0).endPosition(5)).build();
	tick(held);  // start at t=0
	advanceTestClock(5000);
	tick(held);
	EXPECT_EQ(held->state, LedPipelineRunningState::RUNNING) << "default Moving holds at endPosition, defers to inner";
	delete held;

	// Opt out: finishes when the move completes.
	resetTestClock();
	auto *moving =
		SpyEffect::Builder().wrap(Moving::Builder(1000).startPosition(0).endPosition(5).terminateOnComplete(true)).build();
	tick(moving);  // start at t=0
	advanceTestClock(500);
	tick(moving);
	EXPECT_EQ(moving->state, LedPipelineRunningState::RUNNING) << "mid-move, not done yet";

	advanceTestClock(500);  // total elapsed = 1000 == runtime, move complete
	tick(moving);
	EXPECT_EQ(moving->state, LedPipelineRunningState::DONE) << "terminateOnComplete Moving finishes when move lands";
	delete moving;
}
