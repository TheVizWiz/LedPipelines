#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

TEST(TemporaryLedDataBracketTest, AssignSetsColorAtFullOpacity) {
	setUpLeds();

	TemporaryLedData data;
	data[3] = RGBA(RGBA::Red);

	RGBA got = data[3];
	EXPECT_EQ(got.r, 255);
	EXPECT_EQ(got.g, 0);
	EXPECT_EQ(got.b, 0);
	EXPECT_EQ(data.getOpacity(3), 255) << "bracket assignment should default to fully opaque, like set()";
}

TEST(TemporaryLedDataBracketTest, BracketMatchesSet) {
	setUpLeds();

	TemporaryLedData viaBracket;
	TemporaryLedData viaSet;
	viaBracket[5] = RGBA(RGBA::Blue);
	viaSet.set(5, RGBA(RGBA::Blue));

	EXPECT_EQ(RGBA(viaBracket[5]), RGBA(viaSet[5]));
	EXPECT_EQ(viaBracket.getOpacity(5), viaSet.getOpacity(5));
}

// The stored color is not pre-multiplied by opacity; opacity is applied later, at populate() time.
TEST(TemporaryLedDataBracketTest, AssignWithExplicitOpacity) {
	setUpLeds();

	TemporaryLedData data;
	data[2] = RGBA(10, 20, 30, 128);

	EXPECT_EQ(data.getOpacity(2), 128) << "the RGBA's own alpha should be stored as opacity";
	RGBA got = data[2];
	EXPECT_EQ(got.r, 10);
	EXPECT_EQ(got.g, 20);
	EXPECT_EQ(got.b, 30);
}

TEST(TemporaryLedDataBracketTest, UnsetAndOutOfRangeReadsAreSafe) {
	setUpLeds();

	TemporaryLedData data;
	EXPECT_EQ(data.getOpacity(0), 0) << "freshly cleared buffer is transparent";

	RGBA off = data[test_helpers::kLedCount + 1000];
	EXPECT_EQ(off.r, 0);
	EXPECT_EQ(off.g, 0);
	EXPECT_EQ(off.b, 0);
}
