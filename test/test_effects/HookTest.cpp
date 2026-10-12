#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include "../utils/test_helpers.h"

namespace {
	void tick(LedPipelineStage *stage) {
		TemporaryLedData data;
		stage->calculate(0, data);
	}

	using State = LedPipelineRunningState;
}

TEST(HookTest, FiresEveryFrameWithInnerState) {
	setUpLeds();
	resetTestClock();

	std::vector<State> seen;
	auto *hook = Solid::Builder(RGBA::Red)
					 .wrap(FadeOut::Builder(1000))
					 .wrap(Hook::Builder().callback([&](Hook::CallbackArgs args) { seen.push_back(args.state); }))
					 .build();
	hook->reset();

	tick(hook);
	advanceTestClock(1000);
	tick(hook);
	tick(hook);

	ASSERT_EQ(seen.size(), 3u);
	EXPECT_EQ(seen[0], State::RUNNING);
	EXPECT_EQ(seen[1], State::DONE);
	EXPECT_EQ(seen[2], State::DONE);
	EXPECT_EQ(hook->state, State::DONE);

	delete hook;
}

TEST(HookTest, MissingCallbackIsAPassthrough) {
	setUpLeds();
	resetTestClock();

	auto *hook = Solid::Builder(RGBA::Red).wrap(FadeOut::Builder(1000)).wrap(Hook::Builder()).build();
	auto *timedHook = Solid::Builder(RGBA::Red).wrap(FadeOut::Builder(1000)).wrap(TimedHook::Builder()).build();
	auto *stateHook = Solid::Builder(RGBA::Red).wrap(FadeOut::Builder(1000)).wrap(StateHook::Builder()).build();

	for (LedPipelineStage *stage : {(LedPipelineStage *)hook, (LedPipelineStage *)timedHook, (LedPipelineStage *)stateHook}) {
		stage->reset();
		tick(stage);
		EXPECT_EQ(stage->state, State::RUNNING);
	}

	delete hook;
	delete timedHook;
	delete stateHook;
}

TEST(TimedHookTest, ReportsTimelineOfDirectlyWrappedTimedEffect) {
	setUpLeds();
	resetTestClock();

	float pct = -1;
	unsigned long runtime = 0;
	auto *hook = Solid::Builder(RGBA::Red)
					 .wrap(FadeOut::Builder(1000))
					 .wrap(TimedHook::Builder().callback([&](TimedHook::CallbackArgs args) {
						 pct = args.elapsedPercentage;
						 runtime = args.runtimeMs;
					 }))
					 .build();
	hook->reset();

	tick(hook);
	advanceTestClock(500);
	tick(hook);

	EXPECT_FLOAT_EQ(pct, 0.5f);
	EXPECT_EQ(runtime, 1000ul);

	delete hook;
}

TEST(TimedHookTest, FindsTimedEffectThroughIntermediateWrappers) {
	setUpLeds();
	resetTestClock();

	float pct = -1;
	unsigned long runtime = 0;
	auto *hook = Solid::Builder(RGBA::Red)
					 .wrap(FadeOut::Builder(1000))
					 .wrap(Shift::Builder(1))
					 .wrap(Hook::Builder())
					 .wrap(TimedHook::Builder().callback([&](TimedHook::CallbackArgs args) {
						 pct = args.elapsedPercentage;
						 runtime = args.runtimeMs;
					 }))
					 .build();
	hook->reset();

	tick(hook);
	advanceTestClock(250);
	tick(hook);

	EXPECT_FLOAT_EQ(pct, 0.25f);
	EXPECT_EQ(runtime, 1000ul);

	delete hook;
}

TEST(TimedHookTest, ReportsZeroWhenNoTimedEffectIsReachable) {
	setUpLeds();
	resetTestClock();

	float pct = -1;
	unsigned long runtime = 1;
	auto *hook = Solid::Builder(RGBA::Red)
					 .wrap(Shift::Builder(1))
					 .wrap(TimedHook::Builder().callback([&](TimedHook::CallbackArgs args) {
						 pct = args.elapsedPercentage;
						 runtime = args.runtimeMs;
					 }))
					 .build();
	hook->reset();

	tick(hook);

	EXPECT_FLOAT_EQ(pct, 0.0f);
	EXPECT_EQ(runtime, 0ul);

	delete hook;
}

TEST(StateHookTest, FiresOnlyOnStateChanges) {
	setUpLeds();
	resetTestClock();

	std::vector<std::pair<State, State>> changes;
	auto *hook = Solid::Builder(RGBA::Red)
					 .wrap(FadeOut::Builder(1000))
					 .wrap(StateHook::Builder().callback([&](StateHook::CallbackArgs args) {
						 changes.emplace_back(args.oldState, args.newState);
					 }))
					 .build();
	hook->reset();
	EXPECT_TRUE(changes.empty()) << "resetting a NOT_STARTED stage is not a change";

	tick(hook);
	advanceTestClock(500);
	tick(hook);
	advanceTestClock(500);
	tick(hook);
	tick(hook);

	ASSERT_EQ(changes.size(), 2u);
	EXPECT_EQ(changes[0], std::make_pair(State::NOT_STARTED, State::RUNNING));
	EXPECT_EQ(changes[1], std::make_pair(State::RUNNING, State::DONE));

	hook->reset();
	ASSERT_EQ(changes.size(), 3u);
	EXPECT_EQ(changes[2], std::make_pair(State::DONE, State::NOT_STARTED));

	delete hook;
}

TEST(StateHookTest, ReportsLoopRestartsUnderLoop) {
	setUpLeds();
	resetTestClock();

	std::vector<std::pair<State, State>> changes;
	auto *loop = Solid::Builder(RGBA::Red)
					 .wrap(FadeOut::Builder(1000))
					 .wrap(StateHook::Builder().callback([&](StateHook::CallbackArgs args) {
						 changes.emplace_back(args.oldState, args.newState);
					 }))
					 .wrap(Loop::Builder())
					 .build();
	loop->reset();

	tick(loop);
	advanceTestClock(1000);
	tick(loop);

	ASSERT_EQ(changes.size(), 3u);
	EXPECT_EQ(changes[0], std::make_pair(State::NOT_STARTED, State::RUNNING));
	EXPECT_EQ(changes[1], std::make_pair(State::RUNNING, State::DONE));
	EXPECT_EQ(changes[2], std::make_pair(State::DONE, State::NOT_STARTED));

	delete loop;
}

#ifdef LEDPIPELINES_SERIALIZATION
TEST(HookTest, SerializesAllHookTypes) {
	setUpLeds();

	auto *pipeline = SolidSegment::Builder(RGBA::Blue, 10)
						 .wrap(Hook::Builder().callback([](Hook::CallbackArgs) {}))
						 .wrap(TimedHook::Builder().callback([](TimedHook::CallbackArgs) {}))
						 .wrap(StateHook::Builder().callback([](StateHook::CallbackArgs) {}))
						 .build();

	std::string json = pipeline->toJson(true);

	EXPECT_NE(json.find("\"type\": \"Hook\""), std::string::npos);
	EXPECT_NE(json.find("\"type\": \"TimedHook\""), std::string::npos);
	EXPECT_NE(json.find("\"type\": \"StateHook\""), std::string::npos);
	EXPECT_NE(json.find("\"runningState\""), std::string::npos);

	delete pipeline;
}
#endif
