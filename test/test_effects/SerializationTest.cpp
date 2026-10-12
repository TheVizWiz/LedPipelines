#include <gtest/gtest.h>

#include "../utils/test_helpers.h"

TEST(SerializationTest, ConfigOnlyOmitsState) {
	setUpLeds();

	auto *pipeline = SolidSegment::Builder(RGBA::Blue, 10)
						 .wrap(Moving::Builder(3000).startPosition(0).endPosition(50))
						 .build();

	std::string json = pipeline->toJson(false);

	EXPECT_NE(json.find("\"type\": \"Moving\""), std::string::npos);
	EXPECT_NE(json.find("\"startPosition\""), std::string::npos);
	EXPECT_EQ(json.find("\"state\""), std::string::npos);
	EXPECT_EQ(json.find("runningState"), std::string::npos);

	delete pipeline;
}

TEST(SerializationTest, IncludeStateEmitsStateBlock) {
	setUpLeds();

	auto *pipeline = SolidSegment::Builder(RGBA::Blue, 10)
						 .wrap(Moving::Builder(3000).startPosition(0).endPosition(50))
						 .build();

	std::string json = pipeline->toJson(true);

	EXPECT_NE(json.find("\"state\""), std::string::npos);
	EXPECT_NE(json.find("\"runningState\""), std::string::npos);
	EXPECT_NE(json.find("\"elapsedPercentage\""), std::string::npos);
	EXPECT_NE(json.find("\"startPosition\""), std::string::npos);

	delete pipeline;
}
