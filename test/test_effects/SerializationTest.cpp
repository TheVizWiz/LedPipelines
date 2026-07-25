// Tests for pipeline serialization (LedPipelineStage::toJson). Part of the test_effects suite - no main() here; it
// lives in SharedTest.cpp. The suite builds with -D LEDPIPELINES_SERIALIZATION (see platformio.ini), so toJson is
// available.

#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

// toJson(false) is the config-only view: it must NOT emit a "state" sub-object anywhere in the tree.
TEST(SerializationTest, ConfigOnlyOmitsState) {
	setUpLeds();

	auto *pipeline = SolidSegment::Builder(RGBA::Blue, 10)
						 .wrap(Moving::Builder(3000).startPosition(0).endPosition(50))
						 .build();

	std::string json = pipeline->toJson(false);

	// Config fields are present...
	EXPECT_NE(json.find("\"type\": \"Moving\""), std::string::npos);
	EXPECT_NE(json.find("\"startPosition\""), std::string::npos);
	// ...but no state block, at any level of the tree.
	EXPECT_EQ(json.find("\"state\""), std::string::npos);
	EXPECT_EQ(json.find("runningState"), std::string::npos);

	delete pipeline;
}

// toJson(true) adds a "state" sub-object to each stage, carrying runningState plus (for a timed effect) its live
// timeline. The nested inner stage gets its own state block too.
TEST(SerializationTest, IncludeStateEmitsStateBlock) {
	setUpLeds();

	auto *pipeline = SolidSegment::Builder(RGBA::Blue, 10)
						 .wrap(Moving::Builder(3000).startPosition(0).endPosition(50))
						 .build();

	std::string json = pipeline->toJson(true);

	// The state sub-object exists and carries the base running state.
	EXPECT_NE(json.find("\"state\""), std::string::npos);
	EXPECT_NE(json.find("\"runningState\""), std::string::npos);
	// The wrapping Moving is a timed effect, so its state includes the live timeline.
	EXPECT_NE(json.find("\"elapsedPercentage\""), std::string::npos);
	// Config is still present alongside state.
	EXPECT_NE(json.find("\"startPosition\""), std::string::npos);

	delete pipeline;
}
