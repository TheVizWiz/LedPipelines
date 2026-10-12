#include "LedPipelines.h"
#include "outputs/FastLEDOutput.h"


using namespace ledpipelines;
using namespace ledpipelines::effects;

FastLEDOutput ledOutput;


// #define SWITCH_PIN 13
// #define LEFT_PIN 14
// #define RIGHT_PIN 3
// #define TAIL_PIN 15
// #define RIGHT_COUNT 109
// #define LEFT_COUNT 99
// #define TAIL_COUNT 58


#define LED_COUNT (RIGHT_COUNT + LEFT_COUNT + TAIL_COUNT + 1)

CRGB leds[255];

LedPipelineStage* onPipeline;
LedPipelineStage* offPipeline;

void setup() {
	Serial.begin(115200);

	Serial.println("starting LEDs");

	FastLED.addLeds<WS2812B, 48, RGB>(leds, 1);
	// FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1, LEFT_COUNT);
	// FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1 + LEFT_COUNT, RIGHT_COUNT);
	// FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1 + LEFT_COUNT + RIGHT_COUNT, TAIL_COUNT);

	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(14, OUTPUT);

	ledpipelines::setOutput(&ledOutput);
	ledpipelines::initialize();
	ledpipelines::setMaxRefreshRate(60);

	Serial.print("There are this many leds: ");
	Serial.println(TemporaryLedData::size);

	// offPipeline = ParallelLedPipeline::Builder()
	// 				  .addStage(Solid::Builder(RGBA::Black))
	// 				  .addStage(
	// 					  SeriesLedPipeline::Builder()
	// 						  .addStage(SolidSegment::Builder(RGBA::Blue, 1).wrap(TimeBox::Builder(1000)))
	// 						  .addStage(SolidSegment::Builder(RGBA::Green, 1).wrap(TimeBox::Builder(1000)))
	// 						  .wrap(Loop::Builder())
	// 				  )
	// 				  .build();

	// onPipeline =
	// 	ParallelLedPipeline::Builder()
	// 		.addStage(
	// 			Solid::Builder(RGBA::White)
	// 				.wrap(
	// 					OpacityGradient::Builder(0, TemporaryLedData::size).smoothingFunction(SmoothingFunction::LINEAR)
	// 				)
	// 		)
	// 		.addStage(
	// 			SeriesLedPipeline::Builder()
	// 				.addStage(SolidSegment::Builder(RGBA::Red, 1).wrap(TimeBox::Builder(1000)))
	// 				.addStage(SolidSegment::Builder(RGBA::Red, 1).wrap(TimeBox::Builder(1000)))
	// 				.wrap(Loop::Builder())
	// 		)
	// 		.addStage(
	// 			SolidSegment::Builder(RGBA::White, 10)
	// 				.wrap(OpacityGradient::Builder(1))
	// 				.wrap(OpacityGradient::Builder(1))
	// 				.wrap(
	// 					Moving::Builder(10000).startPosition(0).endPosition(10).smoothingFunction(
	// 						SmoothingFunction::LINEAR
	// 					)
	// 				)
	// 				.wrap(Moving::Builder(10).startPosition(-10).endPosition(LED_COUNT + 10))
	// 				.wrap(Loop::Builder())
	// 		)
	// 		.build();
	//
	// Serial.println("done initializing onPipeline and offPipeline");
	// onPipeline->reset();
	// offPipeline->reset();


	onPipeline = TimedSpawner::Builder(
					 []() {
						 auto sparkleBase = SolidSegment::Builder(RGBA::OrangeRed, 1).shared();

						 auto sparkle =
							 SeriesLedPipeline::Builder()
								 .addStage(sparkleBase.wrap(FadeIn::Builder(1000).terminateOnComplete(true)))
								 .addStage(sparkleBase.wrap(TimeBox::Builder(1000).terminateOnComplete(true)))
								 .addStage(sparkleBase.wrap(FadeOut::Builder(1000).terminateOnComplete(true)))
					 	;
						 return sparkle.build();
					 },
					 500)
					 .maxChildren(1)
					 .keepOldOnSpawn(true)
					 .build();
}

void loop() {
	// bool effectShouldPlay = digitalRead(SWITCH_PIN) == LOW;


	//    onPipeline->dryRun();

	//    if (effectShouldPlay) {
	onPipeline->run();
	//    } else {
	//        offPipeline->run();
	//    }
}
