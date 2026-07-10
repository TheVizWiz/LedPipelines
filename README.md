# LedPipelines

LedPipelines is a library that sits on top of FastLED and provides a way to create animations that can layer easily and
fluidly, with an easy-to-use declarative syntax.

LedPipelines is a library meant to sit on top of FastLED and provide a way to create animation pipelines that can layer
easily and fluidly. It will support animations, layers, multiple segments, fractional LED lighting, and more features.

- [current worked on tickets](https://github.com/users/TheVizWiz/projects/3/views/1) (will try to keep this up to date,
  sorry!)
- [feature requests](https://github.com/TheVizWiz/LedPipelines/issues)

## Preview your pipeline without hardware

You don't need to flash a microcontroller to see what an effect looks like.
[**LedPipelinesViewer**](https://github.com/TheVizWiz/LedPipelinesViewer) is a standalone desktop previewer: write a
pipeline in C++ exactly as you would in an Arduino sketch, build, and watch it animate live in your browser as a grid
of pixels.

It compiles LedPipelines natively against host-side `FastLED.h` / `Arduino.h` stubs — the library is unchanged, but
`FastLED.show()` streams each rendered frame to the browser instead of latching physical LEDs, on a real wall-clock so
timed effects animate at their true speed. What you see is the exact RGB the physical LEDs would receive.

It's the fastest way to iterate on an effect: tweak your pipeline, rebuild, and see the result in a second or two. See
the [LedPipelinesViewer repository](https://github.com/TheVizWiz/LedPipelinesViewer) for setup and usage.

## Installation (PlatformIO)

LedPipelines is published to the PlatformIO registry as `thevizwiz/LedPipelines`. Add it to your project's
`platformio.ini` under `lib_deps` — FastLED comes in automatically as a declared dependency:

```ini
[env:esp32]
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino
lib_deps =
    thevizwiz/LedPipelines

; LedPipelines requires C++20 (gnu++2a). The Arduino framework defaults to
; gnu++11, so unset it and opt into the newer standard.
build_unflags =
    -std=gnu++11
build_flags =
    -std=gnu++2a
```

To pin a version, use `thevizwiz/LedPipelines@^0.1.4`. To build against an unreleased local checkout instead (e.g. while
developing effects), point `lib_deps` at the working tree:

```ini
lib_deps =
    symlink:///absolute/path/to/ledpipelines
```

## Why?

WLED, and similar libraries, suck when you want to implement more than one effect at a time. They generally also aren't
great at reactive effects, or effects that are more complex than your basic effects. LedPipelines solves these problems
by using a pipeline-esque way to handle LED lighting - effects are added as stages in a pipeline, which layer on top of
each other to create the final product.

Check out the examples for a clearer understanding of how this makes building complex effects easier and faster.

## Core concepts

LedPipelines is built out of a few small ideas that compose:

- **Stage** — the unit of everything. Every effect and every pipeline is a `LedPipelineStage`. A stage's job is to write
  color and opacity into a per-frame buffer when its `calculate()` runs.
- **Source** — a stage that *produces* pixels (e.g. `Solid`, `SolidSegment`, `HSVGradient`, `RGBGradient`, `Spawner`,
  `Mask`).
- **Wrapper** — a stage that *wraps another stage* and post-processes or repositions what that inner stage produced (
  e.g. `Moving`, `Loop`, `Shift`, `FadeOut`, `OpacityGradient`). You attach a wrapper with `.wrap(...)`.
- **Pipeline** — a stage that holds *multiple child stages*: `SeriesLedPipeline` runs them one after another over time,
  `ParallelLedPipeline` layers them on top of each other in the same frame. You add children with `.addStage(...)`.
- **Builder** — you never construct stages directly; you configure a `Builder` fluently and call `.build()`, which
  produces a fresh, independent stage tree. `.wrap(...)` and `.addStage(...)` operate on builders too.

Every frame, `pipeline->run()` clears the strip, walks the tree calling `calculate()` on each stage (children/inners
feed their parents), bakes the result into FastLED, and shows it. `run()` self-rate-limits to `setMaxRefreshRate(...)`,
so you can call it as fast as you like.

### The render model (important)

**Nothing persists between frames.** Each frame starts from a cleared buffer and the whole tree is recomputed from
scratch. This is why a fade, a move, or any time-based effect must *wrap* the content it acts on — there is no "previous
frame" to modify. Time-based effects read the real clock (`millis()`), so they animate on their own; you don't advance
them manually.

## Available effects

### Sources (produce pixels)

| Effect         | Builder                                  | What it does                                                                                                                                                                                                                                                                                                                                                                                          |
|----------------|------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Solid`        | `Solid::Builder(color)`                  | Fills the entire strip with one color. `.opacity(0-255)`.                                                                                                                                                                                                                                                                                                                                             |
| `SolidSegment` | `SolidSegment::Builder(color, length)`   | Lights a segment of `length` pixels starting where it's positioned. Fractional lengths feather the end pixels.                                                                                                                                                                                                                                                                                        |
| `HSVGradient`  | `HSVGradient::Builder(startPos, endPos)` | Writes a gradient between two positions, interpolating in **HSV** — the blend travels around the color wheel (red→blue passes through magenta or green). `.startGradient(a, b)` sets the look; add a runtime + `.endGradient(a, b)` to morph one gradient into another over time. Hue is in degrees and wraps at 360 (so `0..720` = two rainbows). Corners are `FHSV`; see [HSV colors](#hsv-colors). |
| `RGBGradient`  | `RGBGradient::Builder(startPos, endPos)` | Same as `HSVGradient`, but interpolates each **RGB channel** directly — a straight crossfade between the two colors through their RGB midpoint (red→blue passes through dim purple, not the wheel). Corners are `CRGB`. Use this for a plain smooth blend; use `HSVGradient` when you want the intermediate hues.                                                                                     |
| `Mask`         | `Mask::Builder(base, mask)`              | Uses one stage's brightness to gate another — `base` shows through only where `mask` is lit.                                                                                                                                                                                                                                                                                                          |
| `Spawner`      | `Spawner::Builder(factory)`              | Periodically spawns child effects from a factory (`[]{ return SomeEffect::Builder(...).build(); }`). Great for sparkles/particles. `RandomSpawner` / timed variants vary the interval.                                                                                                                                                                                                                |

### Wrappers (transform an inner stage — attach with `.wrap(...)`)

**Positional**

| Effect             | Builder                               | What it does                                                                                                           |
|--------------------|---------------------------------------|------------------------------------------------------------------------------------------------------------------------|
| `Shift`            | `Shift::Builder(offset)`              | Statically shifts the inner by a fixed number of pixels.                                                               |
| `Moving`           | `Moving::Builder(runtimeMs)`          | Animates the inner from `.startPosition(x)` to `.endPosition(y)` over `runtimeMs`, easing with `.smoothingFunction(...)`. After the move completes it **holds** the inner at `endPosition` and keeps rendering it — `Moving` finishes only when the inner finishes, not when the timer runs out (wrap a `TimeBox` if the inner never ends on its own). |
| `AbsolutePosition` | `AbsolutePosition::Builder(position)` | Pins the inner to an absolute strip position, ignoring where its parent placed it.                                     |
| `Repeat`           | `Repeat::Builder(repeatDistance)`     | Tiles copies of the inner every `repeatDistance` pixels. `.numRepeats(n)` (0 = fill the strip).                        |
| `Flip`             | `Flip::Builder(maxIndex)`             | Mirrors the inner's *position* within `[minIndex, maxIndex]` (a spatial reversal — left↔right, not a reverse in time). |
| `Path`             | `Path::Builder().addSegment(a, b)...` | Remaps the inner across arbitrary strip segments (e.g. for non-contiguous physical layouts).                           |

**Opacity / color**

| Effect            | Builder                                                          | What it does                                                                                                                               |
|-------------------|------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| `FadeIn`          | `FadeIn::Builder(runtimeMs)`                                     | Ramps the inner's opacity 0→full over `runtimeMs`, then holds. `RandomFadeIn` randomizes the duration.                                     |
| `FadeOut`         | `FadeOut::Builder(runtimeMs)`                                    | Ramps the inner's opacity full→0 over `runtimeMs`. Combine with `.delayMs(t)` to hold, then fade. `RandomFadeOut` randomizes the duration. |
| `OpacityGradient` | `OpacityGradient::Builder(endIndex)` or `(startIndex, endIndex)` | Applies a spatial opacity ramp across the inner. `.smoothingFunction(...)`.                                                                |
| `OpacityScale`    | `OpacityScale::Builder(maxOpacity)`                              | Scales the inner's opacity down to a ceiling.                                                                                              |

**Timing / control flow**

| Effect         | Builder                       | What it does                                                                                                                                                                               |
|----------------|-------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `TimeBox`      | `TimeBox::Builder(runtimeMs)` | Runs the inner for `runtimeMs` then reports DONE. The building block of timed sequences. `RandomTimeBoxedEffect` randomizes it.                                                            |
| `Wait`         | `Wait::Builder(runtimeMs)`    | A source that does nothing for `runtimeMs` then finishes — a pause in a series. `RandomWaitEffect` randomizes it.                                                                          |
| `Loop`         | `Loop::Builder()`             | Restarts the inner when it finishes. `.numLoops(n)` (0 = forever).                                                                                                                         |
| `Toggle`       | `Toggle::Builder()`           | Gates the inner on/off at runtime via `.activate()` / `.deactivate()` — useful for reactive/interactive control.                                                                           |
| `ResetBlocker` | `ResetBlocker::Builder()`     | On `reset()`, only propagates to the inner if the inner is already DONE; otherwise lets it finish its current pass. Lets a sub-effect complete before a surrounding `Loop` can restart it. |
| `Shared`       | `Shared::Builder(sharedPtr)`  | Wraps an already-built stage held via `std::shared_ptr` so the **same instance** can appear in several places in the tree (lifetime managed by the refcount). Unlike every other wrapper, it takes a built stage, not a builder — see [Sharing a sub-effect](#sharing-a-sub-effect). |

Most timed effects share `.delayMs(t)` (a lead-in delay before their own clock starts) and honor
`.smoothingFunction(...)` where an ease makes sense.

### Convenience methods

Every builder has shorthand methods for the most common `.wrap(...)` targets — identical to writing the wrap out by
hand, just terser:

| Method | Equivalent to |
|---|---|
| `.loop()` | `.wrap(Loop::Builder())` (loops forever) |
| `.loop(n)` | `.wrap(Loop::Builder().numLoops(n))` |
| `.timebox(ms)` | `.wrap(TimeBox::Builder(ms))` |
| `.shift(px)` | `.wrap(Shift::Builder(px))` |
| `.block()` | `.wrap(ResetBlocker::Builder())` |
| `.shared()` | builds this chain once and returns a reusable `Shared::Builder` — see [Sharing a sub-effect](#sharing-a-sub-effect) |

So `Solid::Builder(CRGB::Red).timebox(2000).loop()` is the same as
`Solid::Builder(CRGB::Red).wrap(TimeBox::Builder(2000)).wrap(Loop::Builder())`.

### Sharing a sub-effect

Normally every `build()` produces a fresh, independent tree — nothing is shared. `Shared` is the deliberate exception:
it lets one built stage instance appear in several branches. Build the stage once into a `std::shared_ptr`, then wrap it
in a `Shared` per branch:

```cpp
auto dot = SolidSegment::Builder(CRGB::White, 3).buildShared();  // build once -> shared_ptr
pipeline = ParallelLedPipeline::Builder()
    .addStage(Shared::Builder(dot).shift(0))    // both branches reference the SAME dot
    .addStage(Shared::Builder(dot).shift(20))
    .build();
```

Or use `.shared()`, which builds the chain once and hands back a reusable `Shared::Builder` you can wrap into multiple
branches:

```cpp
auto dot = SolidSegment::Builder(CRGB::White, 3).shared();
pipeline = ParallelLedPipeline::Builder()
    .addStage(dot.shift(0))
    .addStage(dot.shift(20))
    .build();
```

Two things to know:

- **`Shared` shares one instance, never copies.** Reusing/moving a `Shared::Builder` always yields another handle to the
  same stage (`Shared::Builder` is intentionally reusable — even after being moved from, it stays valid). If you want two
  *independent* stages, don't share — build the source chain twice.
- **The shared instance's per-run state is shared too.** Within a single frame this is fine for anything whose output is
  a pure function of the current clock and position — a `Solid`, `SolidSegment`, gradient, or a `Moving`/timed effect all
  render correctly from multiple branches in one frame, since each `calculate()` recomputes from scratch. Be careful only
  with effects that **accumulate or mutate state across calls within a frame** (e.g. `Spawner`, which grows a child list
  each `calculate()`), or where a per-branch `reset()` would disturb the other branch — sharing those can misbehave.

### HSV colors

`HSVGradient` and any future HSV effects use `FHSV`, a floating-point HSV color:

```cpp
FHSV(hue, saturation, value)
```

- **`hue`** — degrees. Any value is accepted and wrapped into `[0, 360)` at render, so `500` → `140°` and `0..720`
  sweeps two full rainbows.
- **`saturation`** — `0.0`–`1.0`. `1` = vivid color, `0` = washed out to gray/white.
- **`value`** — `0.0`–`1.0`. `1` = full brightness, `0` = black.

## Quick Start Guide.

LedPipelines are comprised of `stages`. Each `stage` derives from the base class of `LedPipelineStage`.

### A worked example

Register your strip, initialize the library, build a pipeline, then call `run()` every loop. This example layers three
things at once with a `ParallelLedPipeline`: a dim white base wash, a red segment that sweeps back and forth forever,
and a red/green blinker driven by a timed `SeriesLedPipeline`.

```cpp
#include "FastLED.h"
#include "LedPipelines.h"

using namespace ledpipelines;
using namespace ledpipelines::effects;

#define LED_PIN   5
#define LED_COUNT 100

CRGB leds[LED_COUNT];
LedPipelineStage* pipeline;

void setup() {
    // 1. Register your strip(s) with FastLED FIRST.
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);

    // 2. initialize() AFTER addLeds — it reads the LED count.
    ledpipelines::initialize();
    ledpipelines::setMaxRefreshRate(60);

    // 3. Build the pipeline. Layers are stacked with a ParallelLedPipeline;
    //    each layer is itself a stage (optionally wrapped and/or nested).
    pipeline =
        ParallelLedPipeline::Builder()
            // Layer 1: a dim white wash across the whole strip.
            .addStage(
                Solid::Builder(CRGB::White)
                    .wrap(OpacityScale::Builder(40)))

            // Layer 2: a 10px red segment bouncing back and forth forever.
            // A bounce is two sweeps in series — front-to-back, then
            // back-to-front — looped. (Moving controls direction via its
            // start/end positions; to reverse you run the opposite sweep, not
            // a Flip, which mirrors position in space rather than in time.)
            .addStage(
                SeriesLedPipeline::Builder()
                    .addStage(SolidSegment::Builder(CRGB::Red, 10)
                                  .wrap(Moving::Builder(3000)
                                            .startPosition(0)
                                            .endPosition(LED_COUNT)
                                            .smoothingFunction(SmoothingFunction::SINE))
                                  .wrap(TimeBox::Builder(3000)))   // give the sweep a lifetime so the series advances
                    .addStage(SolidSegment::Builder(CRGB::Red, 10)
                                  .wrap(Moving::Builder(3000)
                                            .startPosition(LED_COUNT)
                                            .endPosition(0)
                                            .smoothingFunction(SmoothingFunction::SINE))
                                  .wrap(TimeBox::Builder(3000)))
                    .wrap(Loop::Builder()))           // repeat the bounce indefinitely

            // Layer 3: a red/green blinker built from a timed series.
            .addStage(
                SeriesLedPipeline::Builder()
                    .addStage(SolidSegment::Builder(CRGB::Red, 5)
                                  .wrap(TimeBox::Builder(500)))
                    .addStage(SolidSegment::Builder(CRGB::Green, 5)
                                  .wrap(TimeBox::Builder(500)))
                    .wrap(AbsolutePosition::Builder(LED_COUNT - 5))  // park it at the end
                    .wrap(Loop::Builder()))
            .build();

    pipeline->reset();
}

void loop() {
    // run() clears, recomputes the whole tree, and shows the frame.
    // It self-rate-limits to setMaxRefreshRate, so calling it hot is fine.
    pipeline->run();
}
```

A few things this shows:

- **`.wrap(...)` reads inside-out.** Each segment is *built*, then *moved*, then *time-boxed* — each wrapper acts on
  everything beneath it.
- **Pipelines nest.** Both the bounce (Layer 2) and the blinker (Layer 3) are whole `SeriesLedPipeline`s used as single
  stages inside the `ParallelLedPipeline`.
- **Series = one at a time, Parallel = all at once.** The bounce's two sweeps run one after the other (series), as do
  the blinker's two segments; meanwhile all three layers render into the same frame (parallel).
- **Direction is a property of the sweep, not a wrapper.** To reverse a `Moving`, run the opposite sweep
  (`start`/`end` swapped) — that's why a bounce is a *series of two moves*.
- **`Moving` doesn't end itself; it ends when its inner ends.** When a `Moving` reaches `endPosition` it *holds* the
  inner there and keeps rendering it — the move completing doesn't finish the stage. Since a `SolidSegment` never
  finishes on its own, each sweep is wrapped in a `TimeBox` to give it a lifetime, which is what lets the series
  advance and the `Loop` restart. (Wrap a naturally-finishing inner — a `FadeOut`, an animated `HSVGradient` — and you
  don't need the `TimeBox`.)

### The same bounce, two ways

There's usually more than one way to build a given effect. The bounce above reverses direction by running the *opposite
sweep* on the return trip. You can get the identical result by running the *same* sweep both times and spatially
mirroring the second one with `Flip` — since `Flip` mirrors the rendered buffer every frame, a forward `Moving` wrapped
in `Flip` renders as a backward-travelling sweep:

```cpp
// Equivalent Layer 2: same forward sweep both phases; Flip reverses the return.
SeriesLedPipeline::Builder()
    .addStage(SolidSegment::Builder(CRGB::Red, 10)
                  .wrap(Moving::Builder(3000)
                            .startPosition(0)
                            .endPosition(LED_COUNT)
                            .smoothingFunction(SmoothingFunction::SINE))
                  .wrap(TimeBox::Builder(3000)))
    .addStage(SolidSegment::Builder(CRGB::Red, 10)
                  .wrap(Moving::Builder(3000)
                            .startPosition(0)
                            .endPosition(LED_COUNT)
                            .smoothingFunction(SmoothingFunction::SINE))
                  .wrap(Flip::Builder(LED_COUNT))    // mirror the second sweep -> travels back
                  .wrap(TimeBox::Builder(3000)))
    .wrap(Loop::Builder())
```

Which to prefer is a matter of taste: swapping `start`/`end` states the direction directly, while `Flip` keeps both
phases identical and expresses "and now the other way" as a single wrapper. Note a *single* flipped forward sweep is
**not** a bounce — it's just a permanent right-to-left sweep; you still need the two phases in series to alternate.

See the [`examples/`](examples/) directory for more.

## Constraints

A few things are worth knowing before you build something large:

- **One strip topology per program.** The strip layout (total LED count, per-strip offsets) is captured once in
  `initialize()`, which reads it from FastLED. You must call `initialize()` **after** all your `FastLED.addLeds(...)`
  calls, and you cannot change the layout afterward. The buffer state is global/static, so a single process drives one
  strip configuration.
- **Not thread-safe.** Build and run pipelines from a single thread (the usual Arduino `loop()`). There is no internal
  locking around the shared frame buffer.
- **Memory scales with LED count.** The render buffer is padded on both sides for off-screen movement, so it is roughly
  `3 × ledCount` slots wide (each slot holding a color plus an opacity byte). On memory-tight boards, keep an eye on
  total LED count.
- **Requires C++20 (`gnu++2a`).** The library uses modern C++ features; you must compile it (and your sketch) with
  `-std=gnu++2a`. See the [installation](#installation-platformio) `build_flags` above.
- **Nothing persists between frames.** Every frame is recomputed from a cleared buffer (see
  [the render model](#the-render-model-important)). Any effect that depends on time or on prior state must *wrap* the
  content it acts on — it cannot reach back to a previous frame.
- **Timing is real wall-clock.** Timed effects (`Moving`, `TimeBox`, `FadeIn`/`FadeOut`, `Loop` cycles, animated
  `HSVGradient`) advance off `millis()`. They animate on their own; you do not — and should not — advance them manually.
- **`Loop` needs a terminating inner.** `Loop` restarts its inner only once that inner reports DONE. Wrapping an inner
  that never finishes (e.g. a bare `Solid`) in a `Loop` will simply run it forever without ever looping.

## Contributions

Contributions are welcome!

- **Bugs and feature requests** — open an issue on the
  [issue tracker](https://github.com/TheVizWiz/LedPipelines/issues). For features, a short description of the effect or
  behavior you want (and, if you can, an example of how you'd like to express it in a pipeline) is a huge help.
- **Pull requests** — fork the repo, branch off `main`, and open a PR against `main`. Please keep changes focused and
  match the surrounding style.
- **New effects** — the easiest way to contribute is a new effect. Most effects are a small `Builder` + a `calculate()`
  that writes into the frame buffer; the existing effects in [`include/effects/`](include/effects/) and
  [`src/effects/`](src/effects/) are the best templates (start from a simple wrapper like `Shift` or a source like
  `SolidSegment`). Export it from `include/effects/LedEffects.h` and add it to the effect list in this README.
- **Building and testing** — the library builds and tests natively (no hardware required) via the `env:local`
  PlatformIO environment, which swaps in host stubs for FastLED/Arduino:

  ```bash
  pio test -e local     # build + run the unit tests on your machine
  ```

  Please make sure the tests pass before opening a PR, and add coverage for new behavior where it makes sense.

## License

LedPipelines is released under the [MIT License](LICENSE).