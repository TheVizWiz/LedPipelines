#include <gtest/gtest.h>

#include <type_traits>

#include "../utils/test_helpers.h"

static_assert(!std::is_constructible_v<ReferenceValue<float>, float &&>, "temporaries would dangle");
static_assert(!std::is_constructible_v<LiveReferenceValue<float>, float &&>, "temporaries would dangle");

TEST(ReferenceValueTest, CopiesTheVariableOnlyWhenSampled) {
	float external = 1;
	ReferenceValue<float> value(external);

	external = 2;
	EXPECT_FLOAT_EQ(value.get(), 1.0f);

	value.sample();
	EXPECT_FLOAT_EQ(value.get(), 2.0f);
}

TEST(ReferenceValueTest, DeducesTheTypeFromTheVariable) {
	long external = 7;
	auto value = ReferenceValue(external);
	static_assert(std::is_same_v<decltype(value), ReferenceValue<long>>);
	EXPECT_EQ(value.get(), 7L);
}

TEST(ReferenceValueTest, OwningEffectResamplesOnReset) {
	setUpLeds();

	float external = 3;
	auto *shift = SpyEffect::Builder().wrap(Shift::Builder(ReferenceValue(external))).build();
	EXPECT_FLOAT_EQ(shift->offset.get(), 3.0f);

	external = 5;
	EXPECT_FLOAT_EQ(shift->offset.get(), 3.0f);
	shift->reset();
	EXPECT_FLOAT_EQ(shift->offset.get(), 5.0f);

	delete shift;
}

TEST(LiveReferenceValueTest, ReadsTheVariableOnEveryGet) {
	float external = 1;
	LiveReferenceValue<float> value(external);

	external = 2;
	EXPECT_FLOAT_EQ(value.get(), 2.0f);
	EXPECT_FLOAT_EQ(value.value, 2.0f) << "get() refreshes the cached value";
}

TEST(LiveReferenceValueTest, EffectSeesNewValueEveryFrameWithoutReset) {
	setUpLeds();

	RGBA color = RGBA::Red;
	auto *solid = Solid::Builder(LiveReferenceValue(color)).build();

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
TEST(ReferenceValueTest, SerializesAsReferenceProviders) {
	float external = 1;
	EXPECT_EQ(ReferenceValue(external).describeValue(), "{\"provider\" : \"reference\"}");
	EXPECT_EQ(LiveReferenceValue(external).describeValue(), "{\"provider\" : \"liveReference\"}");
}
#endif
