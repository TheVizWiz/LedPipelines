#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

namespace {
	// Renders one frame into a throwaway buffer.
	void tick(LedPipelineStage *stage) {
		TemporaryLedData data;
		stage->calculate(0, data);
	}
}

// SpyEffect never reports DONE, so a FadeIn that defers to it must stay RUNNING forever after the ramp.
TEST(TerminateOnCompleteTest, FadeInDefaultDefersToInnerAndNeverFinishesOnItsOwn) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	auto *fade = SpyEffect::Builder().wrap(FadeIn::Builder(1000)).build();

	// First frame stamps startTimeMs at t=0.
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING);

	advanceTestClock(5000);
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING) << "default FadeIn must defer to its inner, not self-finish";

	delete fade;
}

TEST(TerminateOnCompleteTest, FadeInTerminatesWhenRampCompletes) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	auto *fade = SpyEffect::Builder().wrap(FadeIn::Builder(1000).terminateOnComplete(true)).build();

	tick(fade);  // stamps start at t=0
	advanceTestClock(500);
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING) << "should still be ramping mid-fade";

	advanceTestClock(500);  // total elapsed = 1000 == runtime
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::DONE) << "terminateOnComplete FadeIn must finish at ramp end";

	delete fade;
}

// Guards the applyTiming() wiring: FadeIn::Builder::build() must run the product through applyTiming(), or
// terminateOnComplete (and delayMs) would silently never reach the effect.
TEST(TerminateOnCompleteTest, RandomFadeInTerminatesWhenSampledRampCompletes) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	// min == max so the sampled runtime is deterministic.
	auto *fade = SpyEffect::Builder()
					 .wrap(FadeIn::Builder(RandomRange<unsigned long>(1000, 1000)).terminateOnComplete(true))
					 .build();

	tick(fade);  // stamps start at t=0
	advanceTestClock(999);
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::RUNNING) << "one ms before the sampled ramp end";

	advanceTestClock(1);  // total elapsed = 1000 == sampled runtime
	tick(fade);
	EXPECT_EQ(fade->state, LedPipelineRunningState::DONE)
		<< "terminateOnComplete must propagate through applyTiming to a randomized-duration FadeIn";

	delete fade;
}

TEST(TerminateOnCompleteTest, MovingDefaultHoldsButTerminatesWithFlag) {
	setUpLeds();
	SpyEffect::resetCounters();
	resetTestClock();

	auto *held = SpyEffect::Builder().wrap(Moving::Builder(1000).startPosition(0).endPosition(5)).build();
	tick(held);  // start at t=0
	advanceTestClock(5000);
	tick(held);
	EXPECT_EQ(held->state, LedPipelineRunningState::RUNNING) << "default Moving holds at endPosition, defers to inner";
	delete held;

	resetTestClock();
	auto *moving =
		SpyEffect::Builder().wrap(Moving::Builder(1000).startPosition(0).endPosition(5).terminateOnComplete(true)).build();
	tick(moving);  // start at t=0
	advanceTestClock(500);
	tick(moving);
	EXPECT_EQ(moving->state, LedPipelineRunningState::RUNNING) << "mid-move, not done yet";

	advanceTestClock(500);  // total elapsed = 1000 == runtime
	tick(moving);
	EXPECT_EQ(moving->state, LedPipelineRunningState::DONE) << "terminateOnComplete Moving finishes when move lands";
	delete moving;
}
