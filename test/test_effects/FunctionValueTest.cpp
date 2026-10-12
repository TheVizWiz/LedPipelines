#include <gtest/gtest.h>

#include <type_traits>

#include "../utils/test_helpers.h"

TEST(FunctionValueTest, CallsTheFunctionOnEverySample) {
	int calls = 0;
	FunctionValue<float> value([&]() { return (float)++calls; });

	EXPECT_EQ(calls, 1) << "construction primes the cached value";
	EXPECT_FLOAT_EQ(value.value, 1.0f);

	EXPECT_FLOAT_EQ(value.sample(), 2.0f);
	EXPECT_FLOAT_EQ(value.value, 2.0f);
	EXPECT_EQ(calls, 2);
}

TEST(FunctionValueTest, DeducesTheTypeFromTheReturnType) {
	auto value = FunctionValue([]() { return 7L; });
	static_assert(std::is_same_v<decltype(value), FunctionValue<long>>);
	EXPECT_EQ(value.value, 7L);
}

TEST(FunctionValueTest, OwningEffectResamplesOnReset) {
	setUpLeds();

	float next = 3;
	auto *shift = SpyEffect::Builder().wrap(Shift::Builder(FunctionValue<float>([&]() { return next; }))).build();
	EXPECT_FLOAT_EQ(shift->offset.value, 3.0f);

	next = 5;
	shift->reset();
	EXPECT_FLOAT_EQ(shift->offset.value, 5.0f);

	delete shift;
}

#ifdef LEDPIPELINES_SERIALIZATION
TEST(FunctionValueTest, SerializesAsAFunctionProvider) {
	FunctionValue<float> value([]() { return 1.0f; });
	EXPECT_EQ(value.describeValue(), "{\"provider\" : \"function\"}");
}
#endif

TEST(LiveFunctionValueTest, CallsTheFunctionOnEveryGet) {
	int calls = 0;
	LiveFunctionValue<int> value([&]() { return ++calls; });
	int afterConstruction = calls;

	EXPECT_EQ(value.get(), afterConstruction + 1);
	EXPECT_EQ(value.get(), afterConstruction + 2);
	EXPECT_EQ(value.value, afterConstruction + 2) << "get() refreshes the cached value";
}

TEST(LiveFunctionValueTest, CachedFunctionValueDoesNotRecomputeOnGet) {
	int calls = 0;
	FunctionValue<int> value([&]() { return ++calls; });

	EXPECT_EQ(value.get(), 1);
	EXPECT_EQ(value.get(), 1);
	EXPECT_EQ(calls, 1);
}

TEST(LiveFunctionValueTest, EffectSeesNewValueEveryFrameWithoutReset) {
	setUpLeds();

	RGBA color = RGBA::Red;
	auto *solid = Solid::Builder(LiveFunctionValue<RGBA>([&]() { return color; })).build();

	TemporaryLedData first;
	solid->calculate(0, first);
	EXPECT_EQ(first.get(0), RGBA(RGBA::Red));

	color = RGBA::Blue;
	TemporaryLedData second;
	solid->calculate(0, second);
	EXPECT_EQ(second.get(0), RGBA(RGBA::Blue));

	delete solid;
}

#ifdef LEDPIPELINES_SERIALIZATION
TEST(LiveFunctionValueTest, SerializesAsALiveFunctionProvider) {
	LiveFunctionValue<float> value([]() { return 1.0f; });
	EXPECT_EQ(value.describeValue(), "{\"provider\" : \"liveFunction\"}");
}
#endif
