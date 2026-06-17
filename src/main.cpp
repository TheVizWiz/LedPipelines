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

LedPipelineStage *onPipeline;
LedPipelineStage *offPipeline;

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

	offPipeline = (new ParallelLedPipeline())
			->addStage(new Solid({.color = CRGB::Black}))
			->addStage(
				new Loop(
					(new SeriesLedPipeline())
					->addStage(new TimeBox(new SolidSegment({CRGB::Blue, 1}), {1}))
					->addStage(new TimeBox(new SolidSegment({CRGB::Green, 1}), {1}))
				)
			);

	onPipeline = (new ParallelLedPipeline())
			->addStage((new Solid({.color = CRGB::White}))
				->wrap<OpacityGradient>(OpacityGradient::Config{
					.startIndex = 0,
					.endIndex = TemporaryLedData::size,
					.smoothingFunction = SmoothingFunction::LINEAR
				})
			)
			->addStage(
				new Loop(
					(new SeriesLedPipeline())
					->addStage(
						(new SolidSegment({.length = 1, .color = CRGB::Red}))
						->wrap<TimeBox>(TimeBox::Config{1})
					)
					->addStage(
						(new SolidSegment({.length = 1, .color = CRGB::Red}))
						->wrap<TimeBox>(TimeBox::Config{1})
					)
				)
			)
			->addStage((new SolidSegment({
							.length = 10,
							.color = CRGB::White,
							.opacity = 0xFF
						}
					)
				)
				->wrap<OpacityGradient>(OpacityGradient::Config{
						.endIndex = 1
					}
				)
				->wrap<OpacityGradient>(OpacityGradient::Config{
						.endIndex = 1
					}
				)
				->wrap<Moving>(Moving::Config{
						.runtimeMs = 10000,
						.startPosition = 0,
						.endPosition = 10,
						.smoothingFunction = SmoothingFunction::LINEAR,
					}
				)
				->wrap<Moving>(Moving::Config{
						.runtimeMs = 10,
						.startPosition = -10,
						.endPosition = LED_COUNT + 10
					}
				)
				->wrap<Loop>()
			);

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