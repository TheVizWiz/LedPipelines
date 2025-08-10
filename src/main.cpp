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

BaseLedPipelineStage *onPipeline;
BaseLedPipelineStage *offPipeline;

void setup() {

    Serial.begin(115200);

    Serial.println("starting LEDs");

    FastLED.addLeds<WS2812B, 48, GRB>(leds, 1);
    FastLED.addLeds<WS2812B, LEFT_PIN, RGB>(leds, 1, LEFT_COUNT);
    FastLED.addLeds<WS2812B, RIGHT_PIN, RGB>(leds, 1 + LEFT_COUNT, RIGHT_COUNT);
    FastLED.addLeds<WS2812B, TAIL_PIN, RGB>(leds, 1 + LEFT_COUNT + RIGHT_COUNT, TAIL_COUNT);

    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    LPLogger::initialize(LogLevel::LOG);

    ledpipelines::initialize();
    ledpipelines::setMaxRefreshRate(60);

    Serial.print("There are this many leds: ");
    Serial.println(TemporaryLedData::size);

    onPipeline = (new ParallelLedPipeline())
            ->addStage(new SolidEffect(CRGB::White, 200))
            ->addStage(
                    new LoopEffect(
                            (new SeriesLedPipeline())
                                    ->addStage(
                                            (new SolidSegmentEffect(CRGB::Red, 1))
                                                    ->wrap<TimeBoxedEffect>(1)
                                    )
                                    ->addStage(
                                            (new SolidSegmentEffect(CRGB::Red, 1))
                                                    ->wrap<TimeBoxedEffect>(1)
                                    )
                    )
            )
            ->addStage((new SolidSegmentEffect(CRGB::White, 5))
                               ->wrap<OpacityGradientEffect>(2, 0)
                               ->wrap<OpacityGradientEffect>(-2, 5)
                               ->wrap<MovingEffect>(MovingEffect::Config{
                                       .runtimeMs           = 10000,
                                       .startPosition       = 0,
                                       .endPosition         = 10,
                                       .smoothingFunction   = SmoothingFunction::LINEAR,
                               })
                               ->wrap<MovingEffect>(10, -10, LED_COUNT + 10)
                               ->wrap<LoopEffect>()
            );

    offPipeline = (new ParallelLedPipeline())
            ->addStage(new SolidEffect(CRGB::Black))
            ->addStage(
                    new LoopEffect(
                            (new SeriesLedPipeline())
                                    ->addStage(new TimeBoxedEffect(new SolidSegmentEffect(CRGB::Blue, 1), 1))
                                    ->addStage(new TimeBoxedEffect(new SolidSegmentEffect(CRGB::Green, 1), 1))
                    )
            );


    Serial.println("done initializing onPipeline and offPipeline");
    onPipeline->reset();
    offPipeline->reset();
}

void loop() {

    bool effectShouldPlay = digitalRead(SWITCH_PIN) == LOW;

    if (effectShouldPlay) {
        onPipeline->run();
//        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        offPipeline->run();
//        digitalWrite(LED_BUILTIN, LOW);
    }
}
