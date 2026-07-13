# LedPipelines — Session Summary

A running record of work done on the **LedPipelines** library (FastLED-based LED animation
library for ESP32) and its companion **web previewer** (`../ledpipelineviewer`). This captures
what was built, changed, and decided so a fresh chat can pick up with full context.

> Note: an earlier stretch of work on getting the viewer to build on Windows is intentionally
> omitted from this summary.

---

## Projects

- **Library:** `/Users/vsi/personal/ledpipelines` — the C++ effect library (headers in `include/`,
  sources in `src/`, tests in `test/`). Version `0.1.7` (`library.json`).
- **Viewer:** `/Users/vsi/personal/ledpipelineviewer` — a standalone desktop previewer that
  compiles the library natively against host stubs and streams frames to a browser over SSE. It
  depends on the library (via PlatformIO `lib_deps`), never the other way around.

---

## Effects added / changed this session

### HSVGradient (source)
- Writes a color gradient between two positions, interpolating in **HSV** (hue travels around the
  color wheel). Hue is in degrees and **wraps at 360** (e.g. `0..720` = two full rainbows).
- Uses an `FHSV` struct (float HSV: `h` in degrees, `s`/`v` in `0..1`).
- Supports a **2×2 corner model** (position axis × time axis, bilinear interpolation): four `FHSV`
  corners set via `.startGradient(a, b)` / `.endGradient(a, b)`. With a `runtimeMs`, one gradient
  morphs into another over time and **holds** on `endGradient` at the end.
- `runtimeMs == 0` (the default) = static, infinite, pinned to `startGradient`.
- Pixels outside the range are left untouched (like SolidSegment).

### RGBGradient (source)
- Twin of `HSVGradient` but takes `CRGB` corners and interpolates each **RGB channel** directly —
  a straight crossfade through the RGB midpoint (not around the HSV wheel). Same 2×2 corner model.

### Shared (wrapper)
- Wraps an **already-built stage** held via `std::shared_ptr` (not a Builder), so the **same
  instance** can appear in multiple places in the effect tree — turning the tree into a graph.
  Lifetime is managed by the shared_ptr refcount; `Shared` does not `delete` its inner.
- `Builder(std::shared_ptr<LedPipelineStage>)`. There is no CTAD from a raw pointer, so callers
  must write `std::shared_ptr<LedPipelineStage>(ptr)` explicitly.
- `.shared()` convenience method = builds this chain **once** into a shared_ptr and wraps it in a
  `Shared::Builder` (i.e. `Shared::Builder(buildShared())`). Reusing that one builder in multiple
  branches yields multiple `Shared` stages all backed by the single built instance.

### RandomShift (wrapper)
- Twin of `Shift`, but the offset is **sampled once** from `[minOffset, maxOffset]` on first
  `calculate()` (via the shared `SamplingFunction`), instead of being fixed at build time.
- Re-samples on `reset()` (so a looped RandomShift re-rolls each run).
- Deliberately **not** a `RandomTimedEffect` — it randomizes a *position*, not a *duration* — but
  reuses the same `SamplingFunction` primitive.

### terminateOnComplete flag (on all timed effects)
- New `bool terminateOnComplete` field on `TimedEffect` (default `false`), settable on every timed
  effect's builder and plumbed through `applyTiming()` alongside `delayMs`.
- When set, effects that normally **hold and defer their DONE to their inner** (`FadeIn`,
  `RandomFadeIn`, `Moving`) instead finish the instant their own ramp/move completes.
- Effects that already terminate on their own timer (`FadeOut`, `Wait`, `TimeBox`) are unaffected.
- Motivating use case: sequencing a **random fade-in straight into a random fade-out** in a
  `SeriesLedPipeline` without needing to know the (randomly sampled) fade-in duration ahead of
  time — the Series advances when the fade-in reports DONE.

### Moving — "Model B"
- `Moving` now **holds the inner at `endPosition`** once the move completes and finishes only when
  the *inner* finishes (not when the move timer runs out). `terminateOnComplete(true)` opts out of
  this (end when the move lands).

---

## Renames (naming consistency across the `Random*` family)

- `RandomTimeBoxedEffect` → **`RandomTimeBox`**
- `RandomWaitEffect` → **`RandomWait`**

(`Wait` and `TimeBox` themselves were already suffix-free. The full family is now `RandomFadeIn`,
`RandomFadeOut`, `RandomWait`, `RandomTimeBox`, `RandomTimedSpawner`, plus the base
`RandomTimedEffect`.)

---

## Builder system changes

- **Builders are now copyable** (deep-copy via a polymorphic `clone()`), so a plain builder is a
  **reusable recipe**: `ball.wrap(A); ball.wrap(B)` produces two independent trees and leaves the
  original intact. `std::move` is used for explicit move.
- **`wrap()`** split into `const&` (copies via `clone()`) and `&&` (moves) overloads.
- **`addStage()`** made symmetric with `wrap()` (copy lvalue, move rvalue) for pipeline builders.
- The copyability refactor touched only two base classes; leaf effect builders get it for free via
  CRTP + default copy. Only builders owning `unique_ptr` members needed custom copy.
- **Convenience methods** on the base builder: `.loop()`, `.loop(n)`, `.timebox(ms)`, `.shift(px)`,
  `.block()`, `.shared()` — each a shorthand for the corresponding `.wrap(...)`.

---

## Bugs found & fixed

- **`SamplingFunction::CENTERED` was mathematically wrong.** It computed
  `tanh(5*(x-0.5)) / tanh_range`, which spans `[-0.5, 0.5]` instead of `[0, 1]`, mapping samples to
  `[min - range/2, min + range/2]` — i.e. **near-zero and negative durations**. This made random
  fades "pop" instead of ramp. Fixed by adding the missing `- tanh_min` shift:
  `(tanh(5*(x-0.5)) - tanh_min) / tanh_range`, verified to map `[0,1] → [0,1]` biased to center.
- **`RandomFadeIn::Builder::build()` never called `applyTiming()`** — the only Random builder that
  forgot to, so `delayMs` *and* `terminateOnComplete` were silently dropped on `RandomFadeIn`.
  Fixed to wrap the product in `applyTiming(...)`.
- **`Shift` / `RandomShift` never propagated their inner's DONE state.** They stayed `RUNNING`
  forever, so a `Shift`-wrapped terminating child inside a `Spawner` was never reaped → children
  piled up to `maxChildren` and spawning stopped. Fixed to pass through `this->stage->state`.
  `RandomShift` also got an explicit `bool sampled` flag so sampling is decoupled from the
  passed-through state (an inner that never leaves NOT_STARTED, like a bare `Solid`, would otherwise
  re-sample every frame).
- **`initialize()` / `ledCount()` were non-inline free functions in `LedPipelines.h`** — an ODR /
  duplicate-symbol bug that breaks any project including the header in more than one `.cpp`. Fixed
  by marking them `inline`. (Surfaced when a second test `.cpp` was added.)

---

## Portability fixes (standard-conformance hardening)

The library previously relied on lenient/transitive behavior from the macOS and ESP32 toolchains.
These changes make it self-contained and standard-conforming so it compiles under stricter standard
libraries. Committed in `4d41535` and `7d3ad91`.

- **`u_int64_t` → `uint64_t`** in `include/BaseLedPipeline.h` (`lastUpdateTimeMicros`). `u_int64_t`
  is a BSD/glibc typedef, not standard C++. Also added `#include <cstdint>`.
- **`M_PI` → a local `constexpr float kPi`** in `src/enums/SmoothingFunction.cpp`. `M_PI` is a
  POSIX/glibc extension, not standard, and its availability from `<cmath>` is not guaranteed.
- **Explicit standard includes** added to files that named std types without including their header
  (previously satisfied only transitively):
  - `include/BaseLedPipeline.h`: `<algorithm>`, `<cstdint>`, `<memory>`, `<utility>`, `<vector>`
    (and corrected `#include "memory"` → `<memory>`).
  - `include/effects/BaseEffect.h`: `<memory>`, `<utility>`.
  - `include/effects/Spawner.h`: `<functional>`, `<vector>`.
  - `include/effects/Path.h`: `<utility>`, `<vector>`.
  - `src/effects/Spawner.cpp`: `<algorithm>` (`std::remove_if`).
  - `src/effects/Flip.cpp`: `<algorithm>` (`std::min` / `std::max`).
  - `<cmath>` added to genuine users: `include/enums/SamplingFunction.h` (was quoted `"math.h"`),
    `src/enums/SamplingFunction.cpp`, `src/TemporaryLedData.cpp`, `src/effects/OpacityGradient.cpp`.
- **`String` mock constructor made unambiguous** (`test/mocks/Arduino.h`): replaced the discrete
  `int`/`unsigned int`/`long`/`unsigned long`/`float`/`double` constructors with a single templated
  `is_arithmetic` constructor routing through `std::to_string`. The discrete set was ambiguous for
  types with no exact match (e.g. `uint8_t`, `uint64_t`) on data models where `long` differs in
  width from `unsigned long`.

A systematic audit (non-standard math constants, BSD integer typedefs, POSIX-only headers/functions,
GNU-only math funcs, missing `<cmath>` users, GCC/clang-only pragmas/attributes) came back clean
after these fixes.

## Tests

- Test suites mirror the source layout under `test/`:
  - `test/test_base_utils/` — builder / factory / reuse tests (`BaseLedPipelineTest.cpp`, owns the
    suite's `main()`).
  - `test/test_effects/` — effect tests (`SharedTest.cpp` owns the suite's `main()`;
    `TerminateOnCompleteTest.cpp` added alongside, no own `main()`).
  - `test/utils/test_helpers.h` — shared inline fixtures (`SpyEffect` with live/created counters,
    `setUpLeds()`). Not a suite itself (no `test_` prefix).
- **`TerminateOnCompleteTest.cpp`** (new): 4 tests — FadeIn defaults to deferring forever, FadeIn
  terminates at ramp end with the flag, RandomFadeIn terminates at its sampled ramp end (also guards
  the `applyTiming` fix), and Moving default-holds vs. terminates-with-flag.
- **Test clock control:** added `resetTestClock()` / `advanceTestClock(ms)` to `test/mocks_impl.cpp`
  (declared in `test/mocks/Arduino.h`) so timing tests are deterministic — the "injectable clock"
  the mock had anticipated.
- All suites pass (`pio test -e local`): 13 test cases total.

---

## Documentation

- **Library `README.md`** substantially expanded/maintained: effect tables (Sources, Wrappers,
  Convenience methods), core concepts, the render model, a worked bounce example, Installation
  (PlatformIO), Constraints, Contributions, HSV/FHSV docs, and a "Preview your pipeline" section
  linking the viewer. Updated this session for the renames, `RandomShift`, `terminateOnComplete`,
  and a correction (`RandomSpawner` → `TimedSpawner` / `RandomTimedSpawner`).
- **`LICENSE`** — MIT, "Copyright (c) 2026 Vibhu Iyer".
- **Example `examples/004_shared_bouncing_ball/`** — a bouncing-ball tutorial demonstrating the
  progression from nested `Moving` (messy offset math) → two-stage Series → `Shared` (one ball
  reused in both strokes). Its comments were reviewed and corrected this session (offset-math
  explanation, the `block()`-vs-`Shared` distinction, the `.shared()` API description, and the
  color-over-time vs spatial-gradient wording).

---

## Formatting / tooling notes

- `.clang-format` uses `AlignAfterOpenBracket: Align` (LLVM default), `BinPackParameters/Arguments:
  false`, tabs (`UseTab: Always`, width 4), `ColumnLimit: 120`. This gives the "first arg on the
  open-paren line, chain single-indented" look the user likes.
- Key clang-format facts established: it ignores manual line breaks *within* a statement (only blank
  lines between statements are preserved); whether a method chain cascades (break-before-every-dot)
  depends on the chain's width vs. `ColumnLimit`, not on how it's written; `ColumnLimit` is a target,
  not a hard cap (it can overflow when there's no legal break, or when penalties favor it).
- clang-format is effectively the only mature C++ formatter; Kotlin-style formatting can't be
  applied to C++ (ktlint/ktfmt are Kotlin-only; those rules aren't expressible in clang-format).

---

## Render model reminders (for context)

- Nothing persists between frames: `run()` clears, recomputes the whole tree, bakes to FastLED,
  shows. Effects that depend on time/state must *wrap* the content they act on.
- `run()` self-rate-limits via real wall-clock `millis()`/`micros()`.
- A `SeriesLedPipeline` advances to the next stage when the current stage reports DONE.
- `Loop` needs a terminating inner (a bare `Solid` never finishes, so it never loops).
