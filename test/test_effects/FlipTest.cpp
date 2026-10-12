#include <gtest/gtest.h>

#include <vector>

#include "../utils/test_helpers.h"

namespace {
	struct IndexedEffect : LedPipelineStage {
		void calculate(float, TemporaryLedData &tempData) override {
			for (int i = 0; i < test_helpers::kLedCount; i++) tempData.set(i, RGBA(i, 0, 0));
		}

#ifdef LEDPIPELINES_SERIALIZATION
		std::string toJson(bool) override { return "{}"; }
#endif

		struct Builder : LedPipelineStage::Builder<IndexedEffect, Builder> {
			IndexedEffect *build() override { return new IndexedEffect(); }
		};
	};

	std::vector<int> renderFlipped(long minIndex, long maxIndex) {
		auto *flip = IndexedEffect::Builder().wrap(Flip::Builder(maxIndex).minIndex(minIndex)).build();
		TemporaryLedData data;
		flip->calculate(0, data);
		std::vector<int> reds;
		for (int i = 0; i < test_helpers::kLedCount; i++) reds.push_back(data.get(i).r);
		delete flip;
		return reds;
	}
}

TEST(FlipTest, MirrorsAnEvenWidthRange) {
	setUpLeds();
	EXPECT_EQ(renderFlipped(0, 3), (std::vector<int>{3, 2, 1, 0, 4, 5, 6, 7, 8, 9}));
}

TEST(FlipTest, MirrorsAnOddWidthRangeAroundItsCenter) {
	setUpLeds();
	EXPECT_EQ(renderFlipped(2, 6), (std::vector<int>{0, 1, 6, 5, 4, 3, 2, 7, 8, 9}));
}

TEST(FlipTest, AcceptsBoundsInEitherOrder) {
	setUpLeds();
	EXPECT_EQ(renderFlipped(9, 0), (std::vector<int>{9, 8, 7, 6, 5, 4, 3, 2, 1, 0}));
}
