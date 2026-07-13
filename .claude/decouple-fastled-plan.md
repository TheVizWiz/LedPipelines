# Plan: Decouple LedPipelines from FastLED

## Goal

Remove LedPipelines' hard dependency on FastLED so it can drive any LED backend (FastLED on
hardware, ESPHome's light platform, the web viewer, tests) through a small output interface. This
unblocks ESPHome integration, where FastLED's compile-time template pin makes a YAML-configurable
pin otherwise impossible — with the backend abstracted, ESPHome owns the strip (pin/chipset/order/
count all in YAML) and LedPipelines just renders into it.

## Findings (why this is small)

The FastLED coupling surface is tiny and concentrated:

- **Hardware calls** live in exactly two places:
  - `LedPipelineStage::run()` — `FastLED.clear()`, `FastLED.show()`, `data.populateFastLed()`.
  - `TemporaryLedData` — `initialize()` reads topology (`FastLED.count()`, `FastLED[i].size()`);
    `populateFastLed()` writes final pixels (`FastLED[i][j] = ...`).
- **`CRGB`** is used only as a color value type: `.r/.g/.b` (+ `.red/.green/.blue` aliases), five
  named colors (`Black/White/Red/Green/Blue`), and the library's OWN `operator*`/`*=` (defined in
  `LedPipelineUtils`, not FastLED's). No FastLED color machinery (`nscale8`, `blend`, rainbow HSV)
  is used by the library — it has its own `FHSV` and `fhsvToRgb`.
- The **viewer already substitutes FastLED** via header stubs with this exact surface
  (`count()/[i].size()/[i][j]/clear()/show()/addLeds()`), proving the seam. And the viewer's
  `stubs/CRGB.h` is already a complete, standalone, ABI-compatible `CRGB`/`CHSV` with no hardware
  dependency — the vendored color type we need already exists.

## Decisions (agreed)

1. **Introduce a `LedOutput` interface**; ship a default `FastLEDOutput` so existing hardware
   sketches keep working. (Decision: "Design the LedOutput abstraction.")
2. **Vendor a minimal `CRGB`** into the library, dropping the FastLED dependency from core entirely.
   Reuse the viewer's `stubs/CRGB.h` as the canonical color type. (Decision: "Vendor a minimal
   CRGB.") Risk accepted: user effect code calling FastLED-only CRGB methods not present in the
   vendored type would need those methods added — but the vendored viewer CRGB already includes the
   common ones (nscale8, fadeToBlackBy, lerp8, setHSV, %, named colors).
3. **Wiring via a global registered output**, and **`setOutput()` is REQUIRED** — there is NO
   FastLEDOutput default. If no output is registered, `run()`/`initialize()` do nothing (and log an
   error once). This keeps core truly FastLED-free even on the default path. (Decision confirmed.)
4. **`library.json` drops the `fastled/FastLED` dependency entirely.** FastLED is example-only: the
   examples' own `platformio.ini` brings FastLED, and `FastLEDOutput` compiles in their build.
   (Decision confirmed.)

## FINALIZED (supersedes "Open questions" below)

- `setOutput(LedOutput*)` is required; no default backend. `output()` returns the registered pointer
  (nullptr until set). `run()` and `TemporaryLedData::initialize()` guard on null and no-op + log.
- `CRGB`/`CHSV` are promoted from the viewer's `stubs/CRGB.h` into `include/Color.h`, kept in the
  GLOBAL namespace (so effect code's unqualified `CRGB` is unchanged).
- `FastLEDOutput.h` lives in `include/outputs/` but is NOT included by `LedPipelines.h`. Core never
  references FastLED. Hardware users `#include <outputs/FastLEDOutput.h>` explicitly; it compiles
  because their project supplies FastLED.
- `library.json`: remove the FastLED dependency; version bump to 0.2.0.

## Design

### New: `include/Color.h` (vendored CRGB/CHSV)

Promote the viewer's `stubs/CRGB.h` into the library as the canonical color type. Same struct
(union `r/g/b` + `red/green/blue` + `raw[3]`, named colors, `scale8`, `hsv2rgb_rainbow`). The
library's own `operator*`/`*=` stay in `LedPipelineUtils` (NOT redeclared here — the viewer header
already notes this). Keeps memory layout identical to FastLED's CRGB, so `FastLEDOutput` can convert
by field copy (or reinterpret) at the boundary.

### New: `include/LedOutput.h`

```cpp
namespace ledpipelines {
    struct LedOutput {
        virtual ~LedOutput() = default;
        virtual int  stripCount() const = 0;            // -> TemporaryLedData::initialize
        virtual int  stripSize(int strip) const = 0;    // -> TemporaryLedData::initialize
        virtual void clear() = 0;                        // -> run()
        virtual void show()  = 0;                        // -> run()
        virtual void setPixel(int strip, int i, CRGB c) = 0; // -> populate()
    };
}
```

Five methods — the entire coupling surface, derived from the grep.

### New: `include/FastLEDOutput.h` (default backend, the ONLY file that includes FastLED)

```cpp
struct FastLEDOutput : LedOutput {
    int  stripCount() const override { return FastLED.count(); }
    int  stripSize(int s) const override { return FastLED[s].size(); }
    void clear() override { FastLED.clear(); }
    void show()  override { FastLED.show(); }
    void setPixel(int s, int i, CRGB c) override { FastLED[s][i] = ::CRGB(c.r, c.g, c.b); }
};
```

This is the single place that knows FastLED exists. On ESPHome it isn't used at all.

### Wiring: global output with default

In `LedPipelineUtils` (mirrors existing `minMicrosBetweenUpdates` global-state style):

```cpp
namespace ledpipelines {
    void setOutput(LedOutput* output);   // register a backend
    LedOutput& output();                 // returns registered, or a static FastLEDOutput default
}
```

- `LedPipelineStage::run()`: `output().clear()` / `output().show()` / `data.populate(output())`.
- `TemporaryLedData::initialize()`: takes `LedOutput&` (or reads `output()`), uses
  `stripCount()`/`stripSize()`.
- `TemporaryLedData::populateFastLed()` → `populate(LedOutput& out)`: inner loop calls
  `out.setPixel(i, j, bakedColor)`.

Default-to-FastLEDOutput means existing sketches need **zero changes**. (If we DON'T want core to
carry a FastLED default, flip the default to a no-op/null output and require `setOutput()` — decide
below.)

### Files changed

- **New:** `include/Color.h`, `include/LedOutput.h`, `include/FastLEDOutput.h`.
- **Changed:**
  - `src/BaseLedPipeline.cpp` — `run()` uses `output()` (3 lines).
  - `include/TemporaryLedData.h` / `src/TemporaryLedData.cpp` — drop `#include "FastLED.h"`, use
    `LedOutput` for topology + `populate()` (~10 lines).
  - `include/LedPipelineUtils.h` / `src/LedPipelineUtils.cpp` — include `Color.h` instead of
    FastLED; add `setOutput()`/`output()`.
  - `include/LedPipelines.h` — include the new headers.
  - Effect files that `#include "FastLED.h"` (Solid.h, RGBGradient.h, Shared.h) → include `Color.h`.
- **Not changed:** effect *logic* (they use `.r/.g/.b`, named colors, and the library's own
  operators — all preserved by the vendored CRGB). Tests: swap the mock's FastLED for a tiny
  `TestOutput : LedOutput`, or keep the mock and register a FastLEDOutput.
- **Dependency:** `library.json` drops the hard `fastled/FastLED` dependency (or makes it optional /
  example-only). Version bump 0.1.x → **0.2.0**.

### ESPHome payoff

`ESPHomeOutput : LedOutput` wraps an ESPHome `AddressableLight`: `setPixel` writes into its buffer,
`show()` calls the light's `schedule_show()`. Strip pin/chipset/order/count all become ESPHome
`light:` YAML config — the template-pin problem disappears entirely. Core no longer pulls FastLED
into the ESPHome build.

## Open questions before coding

1. **Default output = FastLEDOutput (zero migration) vs. null/required `setOutput()` (core stays
   FastLED-free even in the default path)?** Trade-off: the former keeps existing sketches
   unchanged but makes `FastLEDOutput.h` (and thus FastLED) part of the default include path; the
   latter keeps core truly FastLED-free but every sketch must call `setOutput(&fastLedOutput)` in
   setup().
2. **`library.json`:** drop FastLED dependency entirely, or keep it as an optional/example
   dependency (examples + FastLEDOutput still use it)?
3. **Migration:** confirm 0.2.0 and a short migration note in README ("register an output, or rely
   on the FastLED default").
