#include "liblogger.h"

static LogLevel currentLogLevel = LOG_LEVEL_DEBUG;

// Set the minimum log level
void set_log_level(LogLevel level) {
    currentLogLevel = level;
}

// Logger function
void logger(LogLevel level, const char* module, const char* format, ...) {
    if (level < currentLogLevel) {
        return;
    }

    const char* levelStrings[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    char logMessage[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(logMessage, sizeof(logMessage), format, args);
    va_end(args);

    printf("[%s] %s: %s\n", levelStrings[level], module, logMessage);
}
