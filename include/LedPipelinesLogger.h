#pragma once

#include "Arduino.h"
#include "enums/LogLevel.h"

namespace ledpipelines {
	struct LPLogger {
	private:
		static void logInternal(LogLevel logLevel, String &log);

		static LogLevel internalLogLevel;

	public:
		static void initialize(LogLevel logLevel) { LPLogger::internalLogLevel = logLevel; }

		static void log(String log) { LPLogger::logInternal(LogLevel::LOG, log); }

		static void log(int log) { LPLogger::log(String(log)); }

		static void log(unsigned int log) { LPLogger::log(String(log)); }

		static void log(long log) { LPLogger::log(String(log)); }

		static void log(unsigned long log) { LPLogger::log(String(log)); }

		static void log(char log) { LPLogger::log(String(log)); }

		static void log(float log) { LPLogger::log(String(log)); }

		static void log(double log) { LPLogger::log(String(log)); }

		static void warn(String warn) { LPLogger::logInternal(LogLevel::WARN, warn); }

		static void warn(int warn) { LPLogger::warn(String(warn)); }

		static void warn(unsigned int warn) { LPLogger::warn(String(warn)); }

		static void warn(long warn) { LPLogger::warn(String(warn)); }

		static void warn(unsigned long warn) { LPLogger::warn(String(warn)); }

		static void warn(char warn) { LPLogger::warn(String(warn)); }

		static void warn(float warn) { LPLogger::warn(String(warn)); }

		static void warn(double warn) { LPLogger::warn(String(warn)); }

		static void error(String error) { LPLogger::logInternal(LogLevel::ERROR, error); }

		static void error(int error) { LPLogger::error(String(error)); }

		static void error(unsigned int error) { LPLogger::error(String(error)); }

		static void error(long error) { LPLogger::error(String(error)); }

		static void error(unsigned long error) { LPLogger::error(String(error)); }

		static void error(char error) { LPLogger::error(String(error)); }

		static void error(float error) { LPLogger::error(String(error)); }

		static void error(double error) { LPLogger::error(String(error)); }

		static void debug(String log) { LPLogger::logInternal(LogLevel::Debug, log); }

		static void debug(int log) { LPLogger::debug(String(log)); };

		static void debug(unsigned int log) { LPLogger::debug(String(log)); };

		static void debug(long log) { LPLogger::debug(String(log)); };

		static void debug(unsigned long log) { LPLogger::debug(String(log)); };

		static void debug(char log) { LPLogger::debug(String(log)); };

		static void debug(float log) { LPLogger::debug(String(log)); };

		static void debug(double log) { LPLogger::debug(String(log)); };
	};
}