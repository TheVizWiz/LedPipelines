#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

namespace {
	void tick(LedPipelineStage *stage) {
		TemporaryLedData data;
		stage->calculate(0, data);
	}

	float fadeProgress(Toggle *toggle) {
		return toggle->innerStage()->timedEffect()->elapsedPercentage;
	}

	Toggle *fadingToggle(Toggle::Builder toggle) {
		return Solid::Builder(RGBA::Red).wrap(FadeOut::Builder(1000)).wrap(std::move(toggle)).build();
	}

	using State = LedPipelineRunningState;
}

TEST(ToggleTest, DeactivateFreezesStateByDefault) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder());
	tick(toggle);
	toggle->deactivate();
	advanceTestClock(5000);
	tick(toggle);

	EXPECT_EQ(toggle->state, State::RUNNING);
	delete toggle;
}

TEST(ToggleTest, CompleteOnDeactivateReportsDoneImmediately) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder().completeOnDeactivate(true));
	tick(toggle);
	toggle->deactivate();

	EXPECT_EQ(toggle->state, State::DONE);
	delete toggle;
}

TEST(ToggleTest, CompleteOnDeactivateLetsASeriesMoveOn) {
	setUpLeds();
	resetTestClock();

	Handle<Toggle> toggle;
	auto *series = SeriesLedPipeline::Builder()
					   .addStage(Solid::Builder(RGBA::Red).wrap(Toggle::Builder().completeOnDeactivate(true)).bind(toggle))
					   .addStage(Solid::Builder(RGBA::Blue))
					   .build();

	TemporaryLedData before;
	series->calculate(0, before);
	EXPECT_EQ(before.get(0), RGBA(RGBA::Red));

	toggle->deactivate();
	TemporaryLedData after;
	series->calculate(0, after);
	EXPECT_EQ(after.get(0), RGBA(RGBA::Blue));

	delete series;
}

TEST(ToggleTest, ResetOnActivateRestartsTheInnerByDefault) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder());
	tick(toggle);
	advanceTestClock(500);
	tick(toggle);
	toggle->deactivate();
	advanceTestClock(200);
	toggle->activate();

	EXPECT_EQ(toggle->state, State::NOT_STARTED);
	tick(toggle);
	EXPECT_FLOAT_EQ(fadeProgress(toggle), 0.0f);
	delete toggle;
}

TEST(ToggleTest, WithoutResetOnActivateTheInnerContinues) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder().resetOnActivate(false));
	tick(toggle);
	advanceTestClock(500);
	tick(toggle);
	toggle->deactivate();
	advanceTestClock(200);
	toggle->activate();
	tick(toggle);

	EXPECT_EQ(toggle->state, State::RUNNING);
	EXPECT_FLOAT_EQ(fadeProgress(toggle), 0.7f);
	delete toggle;
}

TEST(ToggleTest, ReactivatingAfterCompleteAlwaysRestartsTheInner) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder().completeOnDeactivate(true).resetOnActivate(false));
	tick(toggle);
	advanceTestClock(500);
	tick(toggle);
	toggle->deactivate();
	ASSERT_EQ(toggle->state, State::DONE);

	toggle->activate();
	EXPECT_EQ(toggle->state, State::NOT_STARTED) << "a stage that reported DONE comes back only through a reset";
	tick(toggle);
	EXPECT_FLOAT_EQ(fadeProgress(toggle), 0.0f);
	delete toggle;
}

TEST(ToggleTest, ActivateAndDeactivateAreNoOpsWithoutATransition) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder().completeOnDeactivate(true));
	tick(toggle);
	advanceTestClock(500);
	tick(toggle);

	toggle->activate();
	tick(toggle);
	EXPECT_FLOAT_EQ(fadeProgress(toggle), 0.5f) << "activating an active toggle must not reset the inner";

	toggle->deactivate();
	toggle->deactivate();
	EXPECT_EQ(toggle->state, State::DONE);
	delete toggle;
}

TEST(ToggleTest, ToggleHonorsBothOptions) {
	setUpLeds();
	resetTestClock();

	auto *toggle = fadingToggle(Toggle::Builder().completeOnDeactivate(true));
	tick(toggle);
	toggle->toggle();
	EXPECT_EQ(toggle->state, State::DONE);
	toggle->toggle();
	EXPECT_EQ(toggle->state, State::NOT_STARTED);
	delete toggle;
}
