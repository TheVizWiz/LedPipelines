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

TEST(TemporaryLedDataMergeTest, NormalOverTransparentKeepsColorAndAlpha) {
	setUpLeds();

	TemporaryLedData base;
	TemporaryLedData layer;
	layer.set(2, RGBA(0, 255, 0), 128);
	base.merge(layer, BlendingMode::NORMAL);

	EXPECT_EQ(base.get(2), RGBA(0, 255, 0, 128)) << "alpha must not be baked into RGB, or populate() applies it twice";
}

TEST(TemporaryLedDataMergeTest, NormalOverOpaqueBlendsColors) {
	setUpLeds();

	TemporaryLedData base;
	TemporaryLedData layer;
	base.set(2, RGBA(255, 0, 0));
	layer.set(2, RGBA(0, 255, 0), 128);
	base.merge(layer, BlendingMode::NORMAL);

	EXPECT_EQ(base.get(2), RGBA(127, 128, 0, 255));
}

TEST(TemporaryLedDataMergeTest, NormalOverSemiTransparentWeightsByContribution) {
	setUpLeds();

	TemporaryLedData base;
	TemporaryLedData layer;
	base.set(2, RGBA(255, 0, 0), 128);
	layer.set(2, RGBA(0, 0, 255), 128);
	base.merge(layer, BlendingMode::NORMAL);

	RGBA got = base.get(2);
	EXPECT_EQ(got.a, 191);
	EXPECT_EQ(got.r, 85) << "red contributes 128 * 127 / 255 of the 191 total alpha";
	EXPECT_EQ(got.b, 171);
}

TEST(TemporaryLedDataMergeTest, HalfAlphaInsideAPipelineRendersAtHalfBrightness) {
	setUpLeds();

	auto *pipeline = ParallelLedPipeline::Builder().addStage(Solid::Builder(RGBA(0, 255, 0, 128))).build();
	TemporaryLedData data;
	pipeline->calculate(0, data);
	data.populate(test_helpers::gOutput);

	EXPECT_EQ(test_helpers::gOutput.pixels[0], RGBA(0, 128, 0));

	delete pipeline;
}

namespace {
	RGBA mergeOne(RGBA base, RGBA layer, BlendingMode mode) {
		TemporaryLedData baseData;
		TemporaryLedData layerData;
		if (base.a) baseData.set(2, base, base.a);
		if (layer.a) layerData.set(2, layer, layer.a);
		baseData.merge(layerData, mode);
		return baseData.get(2);
	}
}

TEST(TemporaryLedDataMergeTest, OverwriteReplacesColorAndAlpha) {
	setUpLeds();
	EXPECT_EQ(mergeOne(RGBA(255, 0, 0), RGBA(0, 255, 0, 128), BlendingMode::OVERWRITE), RGBA(0, 255, 0, 128));
}

TEST(TemporaryLedDataMergeTest, AddSumsVisibleLight) {
	setUpLeds();
	EXPECT_EQ(mergeOne(RGBA(255, 0, 0), RGBA(0, 255, 0, 50), BlendingMode::ADD), RGBA(255, 50, 0, 255))
		<< "a faint green layer adds only its visible light";
	EXPECT_EQ(mergeOne(RGBA(0, 0, 0, 0), RGBA(0, 255, 0, 128), BlendingMode::ADD), RGBA(0, 255, 0, 128));
	EXPECT_EQ(mergeOne(RGBA(255, 0, 0, 128), RGBA(255, 0, 0, 128), BlendingMode::ADD), RGBA(255, 0, 0, 255));
}

TEST(TemporaryLedDataMergeTest, MultiplyTintsByLayerOpacity) {
	setUpLeds();
	EXPECT_EQ(mergeOne(RGBA(255, 255, 255), RGBA(255, 0, 0), BlendingMode::MULTIPLY), RGBA(255, 0, 0, 255));
	EXPECT_EQ(mergeOne(RGBA(255, 255, 255), RGBA(255, 0, 0, 128), BlendingMode::MULTIPLY), RGBA(255, 127, 127, 255));
	EXPECT_EQ(mergeOne(RGBA(0, 0, 0, 0), RGBA(255, 0, 0), BlendingMode::MULTIPLY).a, 0)
		<< "multiplying cannot add coverage where the destination is empty";
}

TEST(TemporaryLedDataMergeTest, MaskGatesOpacityByMaskBrightness) {
	setUpLeds();
	EXPECT_EQ(mergeOne(RGBA(255, 0, 0), RGBA(255, 255, 255), BlendingMode::MASK), RGBA(255, 0, 0, 255));
	EXPECT_EQ(mergeOne(RGBA(255, 0, 0), RGBA(128, 128, 128), BlendingMode::MASK), RGBA(255, 0, 0, 128));
	EXPECT_EQ(mergeOne(RGBA(255, 255, 255), RGBA(0, 0, 255), BlendingMode::MASK), RGBA(255, 255, 255, 255))
		<< "a fully lit colored mask passes the base through untinted";
	EXPECT_EQ(mergeOne(RGBA(255, 0, 0), RGBA(0, 0, 0, 0), BlendingMode::MASK).a, 0) << "unlit mask hides the base";
}
