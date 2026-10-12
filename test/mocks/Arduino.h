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

// Mirrors the ESP32 core's global min()/max() function templates (not macros, so std::min still works). Returns by
// value so mixed-type comparisons don't dangle a reference to a promoted temporary.
template <typename T, typename U>
typename std::common_type<T, U>::type min(T a, U b) { return a < b ? a : b; }
template <typename T, typename U>
typename std::common_type<T, U>::type max(T a, U b) { return a > b ? a : b; }

class String : public std::string {
public:
	String() = default;
	String(const char *s) : std::string(s) {}
	String(const std::string &s) : std::string(s) {}
	String(char c) : std::string(1, c) {}

	// One templated constructor for every arithmetic type instead of a fixed int/long/float/... overload set. The
	// discrete overloads were ambiguous for types with no exact match - e.g. building a String from a uint8_t or a
	// uint64_t, where several integer overloads convert equally well. This is platform-dependent: on macOS
	// unsigned long is 64-bit so uint64_t matched String(unsigned long) exactly, but on Windows (LLP64, long is
	// 32-bit) it does not, so the call became ambiguous. Routing every arithmetic value through std::to_string (which
	// has its own unambiguous per-type overloads) sidesteps that. Promote small integers to a type std::to_string
	// accepts (it has no uint8_t overload) via a common_type with int.
	template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
	String(T v) : std::string(std::to_string(static_cast<typename std::common_type<T, int>::type>(v))) {}
};

inline String operator+(const String &lhs, const String &rhs) { return String(std::string(lhs) + std::string(rhs)); }
inline String operator+(const String &lhs, const char *rhs) { return String(std::string(lhs) + rhs); }
inline String operator+(const char *lhs, const String &rhs) { return String(std::string(lhs) + std::string(rhs)); }
template <typename T> inline String operator+(const String &lhs, T rhs) { return lhs + String(rhs); }

struct FakeSerial {
	void begin(unsigned long) {}
	void print(const String &) {}
	void println(const String &) {}
	void print(const char *) {}
	void println(const char *) {}
};

extern FakeSerial Serial;

// Backed by a fake counter that only advances via delay() or advanceTestClock(), so tests step time deterministically.
unsigned long millis();
unsigned long micros();
void delay(unsigned long);

// Test-only clock control, not part of the real Arduino API. Call resetTestClock() at the start of a timing test so it
// doesn't inherit time from earlier tests.
void resetTestClock();
void advanceTestClock(unsigned long ms);
