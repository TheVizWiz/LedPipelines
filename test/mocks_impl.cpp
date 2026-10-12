#include "Arduino.h"

FakeSerial Serial;

static unsigned long g_fakeMillis = 0;

unsigned long millis() { return g_fakeMillis; }
unsigned long micros() { return g_fakeMillis * 1000; }
void delay(unsigned long ms) { g_fakeMillis += ms; }

void resetTestClock() { g_fakeMillis = 0; }
void advanceTestClock(unsigned long ms) { g_fakeMillis += ms; }
