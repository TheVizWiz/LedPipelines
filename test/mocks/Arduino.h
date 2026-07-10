#pragma once

// Minimal host-side (native) stub of the Arduino core, sufficient to compile and link the LedPipelines library in
// env:local for unit tests. NOT a faithful Arduino implementation - it only provides the slice the library touches:
// the String type, a no-op Serial, and the millis/micros/delay timing calls. Real behavior is exercised on-device.

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

// The real Arduino core transitively provides much of the standard library; the library relies on some of it (e.g.
// std::function in Spawner) being visible via <Arduino.h>. Pull those in here so the stub matches that expectation.

// The ESP32 Arduino core exposes min()/max() as function templates in the global namespace (not macros, so qualified
// std::min in the library still works). Mirror that so bare min()/max() calls in the library resolve on the host.
// Returns by value (via common_type) so mixed-type comparisons don't dangle a reference to a promoted temporary.
template <typename T, typename U>
typename std::common_type<T, U>::type min(T a, U b) { return a < b ? a : b; }
template <typename T, typename U>
typename std::common_type<T, U>::type max(T a, U b) { return a > b ? a : b; }

// Arduino's String maps cleanly onto std::string for our purposes. We only use construction from numbers/C-strings and
// operator+ concatenation (in the logger and colorToHex).
class String : public std::string {
public:
	String() = default;
	String(const char *s) : std::string(s) {}
	String(const std::string &s) : std::string(s) {}
	String(char c) : std::string(1, c) {}
	String(int v) : std::string(std::to_string(v)) {}
	String(unsigned int v) : std::string(std::to_string(v)) {}
	String(long v) : std::string(std::to_string(v)) {}
	String(unsigned long v) : std::string(std::to_string(v)) {}
	String(float v) : std::string(std::to_string(v)) {}
	String(double v) : std::string(std::to_string(v)) {}
};

// Arduino's String supports `str + number` (and mixed String/const char*) returning a String. std::string's operators
// don't cover the numeric right-hand sides the library concatenates (e.g. `"..." + runtimeMs`), so provide them here
// by stringifying the right operand.
inline String operator+(const String &lhs, const String &rhs) { return String(std::string(lhs) + std::string(rhs)); }
inline String operator+(const String &lhs, const char *rhs) { return String(std::string(lhs) + rhs); }
inline String operator+(const char *lhs, const String &rhs) { return String(std::string(lhs) + std::string(rhs)); }
template <typename T> inline String operator+(const String &lhs, T rhs) { return lhs + String(rhs); }

// A no-op Serial so LPLogger's println/print calls link without doing anything on the host.
struct FakeSerial {
	void begin(unsigned long) {}
	void print(const String &) {}
	void println(const String &) {}
	void print(const char *) {}
	void println(const char *) {}
};

extern FakeSerial Serial;

// Timing: tests don't advance a real clock, so these return a monotonic-ish counter defined in the stub .cpp. delay()
// advances that counter, so a test can step time forward deterministically. For tests that assert on elapsed time,
// resetTestClock() zeroes the counter (call at the start of the test so it doesn't inherit time from earlier tests)
// and advanceTestClock() steps it forward without the semantics of a real sleep.
unsigned long millis();
unsigned long micros();
void delay(unsigned long);

// Test-only clock control (defined in mocks_impl.cpp). Not part of the real Arduino API.
void resetTestClock();
void advanceTestClock(unsigned long ms);
