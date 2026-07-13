// Definitions for the native Arduino stubs. Compiled into the env:local test build so the library's globals (Serial,
// timing) resolve at link time. See Arduino.h in this directory. The library no longer depends on FastLED (topology
// and pixel output go through a registered LedOutput - the tests use test_helpers' TestOutput), so no FastLED global
// is defined here.

#include "Arduino.h"

FakeSerial Serial;

// Simple monotonic counter for millis()/micros(). Tests that care about time can be added later with an injectable
// clock; construction/ownership tests don't depend on it.
static unsigned long g_fakeMillis = 0;

unsigned long millis() { return g_fakeMillis; }
unsigned long micros() { return g_fakeMillis * 1000; }
void delay(unsigned long ms) { g_fakeMillis += ms; }

void resetTestClock() { g_fakeMillis = 0; }
void advanceTestClock(unsigned long ms) { g_fakeMillis += ms; }
