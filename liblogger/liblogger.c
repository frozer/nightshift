#include <stdint.h>
#include "liblogger.h"

static LogLevel currentLogLevel = LOG_LEVEL_DEBUG;

// Set the minimum log level
void set_log_level(LogLevel level) {
    currentLogLevel = level;
}

LogLevel get_log_level() {
    return currentLogLevel;
}

char * logLevel2Str(LogLevel level) {
    const char* levelStrings[] = {
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR"
    };
    return levelStrings[level];
}

// Logger function
void logger(LogLevel level, const char* module, const char* format, ...) {
    if (level < currentLogLevel) {
        return;
    }

    char logMessage[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(logMessage, sizeof(logMessage), format, args);
    va_end(args);

    printf("[%s] %s: %s\n", logLevel2Str(level), module, logMessage);
}

void blobToHexStr(char *res, uint8_t *data, int data_length) {
    int offset = 0;
    for (int i = 0; i < data_length; i++) {
        offset += snprintf(res + offset, 3, "%02x", data[i]);
    }

    // Ensure the string is null-terminated
    res[offset] = '\0';
}
