// Definitions for the native FastLED/Arduino stubs. Compiled into the env:local test build so the library's globals
// (FastLED, Serial, timing) resolve at link time. See FastLED.h / Arduino.h in this directory.

#include "FastLED.h"
#include "Arduino.h"

FakeController FastLED;
FakeSerial Serial;

// Simple monotonic counter for millis()/micros(). Tests that care about time can be added later with an injectable
// clock; construction/ownership tests don't depend on it.
static unsigned long g_fakeMillis = 0;

unsigned long millis() { return g_fakeMillis; }
unsigned long micros() { return g_fakeMillis * 1000; }
void delay(unsigned long ms) { g_fakeMillis += ms; }
