// Tests for TemporaryLedData's pixel access, in particular the operator[] PixelRef proxy. Part of the test_base_utils
// suite (no main() here - BaseLedPipelineTest.cpp owns it). Shared fixtures live in test/utils/test_helpers.h.

#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

// data[i] = color writes exactly like set(i, color): the pixel lights at full opacity and reads back as that color.
TEST(TemporaryLedDataBracketTest, AssignSetsColorAtFullOpacity) {
	setUpLeds();

	TemporaryLedData data;
	data[3] = RGBA(RGBA::Red);

	RGBA got = data[3];  // PixelRef -> RGBA via get()
	EXPECT_EQ(got.r, 255);
	EXPECT_EQ(got.g, 0);
	EXPECT_EQ(got.b, 0);
	EXPECT_EQ(data.getOpacity(3), 255) << "bracket assignment should default to fully opaque, like set()";
}

// The bracket setter and set() are equivalent: writing the same color both ways yields the same stored pixel+opacity.
TEST(TemporaryLedDataBracketTest, BracketMatchesSet) {
	setUpLeds();

	TemporaryLedData viaBracket;
	TemporaryLedData viaSet;
	viaBracket[5] = RGBA(RGBA::Blue);
	viaSet.set(5, RGBA(RGBA::Blue));

	EXPECT_EQ(RGBA(viaBracket[5]), RGBA(viaSet[5]));
	EXPECT_EQ(viaBracket.getOpacity(5), viaSet.getOpacity(5));
}

// Assigning an RGBA honors its own alpha as the pixel's opacity: data[i] = RGBA(r, g, b, a) stores opacity a. The
// stored COLOR is untouched by opacity (opacity is applied later, at populate() time); get() returns the pure color.
TEST(TemporaryLedDataBracketTest, AssignWithExplicitOpacity) {
	setUpLeds();

	TemporaryLedData data;
	data[2] = RGBA(10, 20, 30, 128);

	EXPECT_EQ(data.getOpacity(2), 128) << "the RGBA's own alpha should be stored as opacity";
	RGBA got = data[2];
	EXPECT_EQ(got.r, 10);  // color stored as-is, not pre-multiplied by opacity
	EXPECT_EQ(got.g, 20);
	EXPECT_EQ(got.b, 30);
}

// An unset pixel reads back as transparent black (get()'s default), and reading an out-of-range index is safe.
TEST(TemporaryLedDataBracketTest, UnsetAndOutOfRangeReadsAreSafe) {
	setUpLeds();

	TemporaryLedData data;
	EXPECT_EQ(data.getOpacity(0), 0) << "freshly cleared buffer is transparent";

	// Out-of-range read must not crash and returns Black (get() clamps).
	RGBA off = data[test_helpers::kLedCount + 1000];
	EXPECT_EQ(off.r, 0);
	EXPECT_EQ(off.g, 0);
	EXPECT_EQ(off.b, 0);
}
