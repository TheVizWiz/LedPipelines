#pragma once

#include "Arduino.h"

class LogLevel {
public:
	enum LogLevel_ {
		NONE,
		ERROR,
		WARN,
		LOG,
		Debug,
	};

	LogLevel(const LogLevel_ logLevel) : value(logLevel) {
	}

	bool operator<=(const LogLevel &other) const {
		return this->value <= other.value;
	}

	bool operator>=(const LogLevel &other) const {
		return this->value >= other.value;
	}

	bool operator==(const LogLevel &other) const {
		return this->value == other.value;
	}

	bool operator>(const LogLevel &other) const {
		return this->value > other.value;
	}

	bool operator<(const LogLevel &other) const {
		return this->value < other.value;
	}

	String toString() {
		switch (value) {
			case NONE:
				return "NON";
			case Debug:
				return "DEB";
			case ERROR:
				return "ERR";
			case WARN:
				return "WAR";
			case LOG:
				return "LOG";
		}

		return "LOG";
	}

	operator String() {
		return this->toString();
	}

	operator int() {
		return this->value;
	}

private:
	LogLevel_ value;
};