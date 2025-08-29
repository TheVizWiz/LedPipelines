#include "LedPipelinesLogger.h"

using namespace ledpipelines;

LogLevel LPLogger::internalLogLevel = LogLevel::NONE;


static char formattedTimeBuffer[20];

static String getFormattedTime() {
    unsigned long currentMillis = millis();

    unsigned long milliseconds = currentMillis % 1000;
    unsigned long totalSeconds = currentMillis / 1000;
    unsigned long seconds = totalSeconds % 60;
    unsigned long totalMinutes = totalSeconds / 60;
    unsigned long minutes = totalMinutes % 60;
    unsigned long hours = totalMinutes / 60; // Can go beyond 24

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%04lu:%02lu:%02lu:%03lu", hours, minutes, seconds, milliseconds);

    return buffer;
}

void LPLogger::logInternal(LogLevel logLevel, String &log) {
    if (LPLogger::internalLogLevel < logLevel) return;
    Serial.println("[" + getFormattedTime() + "] (" + logLevel.toString() + ") " + log);
}
