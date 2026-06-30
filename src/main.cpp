#include "FastLED.h"
#include "LedPipelines.h"


using namespace ledpipelines;
using namespace ledpipelines::effects;


#define SWITCH_PIN 13
#define LEFT_PIN 14
#define RIGHT_PIN 3
#define TAIL_PIN 15
#define RIGHT_COUNT 109
#define LEFT_COUNT 99
#define TAIL_COUNT 58


#define LED_COUNT (RIGHT_COUNT + LEFT_COUNT + TAIL_COUNT + 1)

CRGB leds[RIGHT_COUNT + LEFT_COUNT + TAIL_COUNT + 50];

LedPipelineStage* onPipeline;
LedPipelineStage* offPipeline;

void setup() {
	Serial.begin(115200);

	Serial.println("starting LEDs");

	FastLED.addLeds<WS2812B, 48, GRB>(leds, 1);
	FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1, LEFT_COUNT);
	FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1 + LEFT_COUNT, RIGHT_COUNT);
	FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1 + LEFT_COUNT + RIGHT_COUNT, TAIL_COUNT);

	pinMode(SWITCH_PIN, INPUT_PULLUP);
	pinMode(LED_BUILTIN, OUTPUT);

	LPLogger::initialize(LogLevel::ERROR);

	ledpipelines::initialize();
	ledpipelines::setMaxRefreshRate(10);

	Serial.print("There are this many leds: ");
	Serial.println(TemporaryLedData::size);

	offPipeline =
		(new ParallelLedPipeline())
			->addStage(Solid::Builder(CRGB::Black).build())
			->addStage(((new SeriesLedPipeline())
							->addStage(SolidSegment::Builder(CRGB::Blue, 1).build()->wrap(TimeBox::Builder(1)))
							->addStage(SolidSegment::Builder(CRGB::Green, 1).build()->wrap(TimeBox::Builder(1))))
						   ->wrap(Loop::Builder()));

	onPipeline =
		(new ParallelLedPipeline())
			->addStage(Solid::Builder(CRGB::White)
						   .build()
						   ->wrap(OpacityGradient::Builder(0, TemporaryLedData::size)
									  .smoothingFunction(SmoothingFunction::LINEAR)))
			->addStage(((new SeriesLedPipeline())
							->addStage(SolidSegment::Builder(CRGB::Red, 1).build()->wrap(TimeBox::Builder(1)))
							->addStage(SolidSegment::Builder(CRGB::Red, 1).build()->wrap(TimeBox::Builder(1))))
						   ->wrap(Loop::Builder()))
			->addStage(
				SolidSegment::Builder(CRGB::White, 10)
					.opacity(0xFF)
					.build()
					->wrap(OpacityGradient::Builder(1))
					->wrap(OpacityGradient::Builder(1))
					->wrap(Moving::Builder(10000)
							   .startPosition(0)
							   .endPosition(10)
							   .smoothingFunction(SmoothingFunction::LINEAR))
					->wrap(Moving::Builder(10).startPosition(-10).endPosition(LED_COUNT + 10))
					->wrap(Loop::Builder()));

	Serial.println("done initializing onPipeline and offPipeline");
	onPipeline->reset();
	offPipeline->reset();
}

void loop() {
	bool effectShouldPlay = digitalRead(SWITCH_PIN) == LOW;


	//    onPipeline->dryRun();

	//    if (effectShouldPlay) {
	onPipeline->run();
	//    } else {
	//        offPipeline->run();
	//    }
}
