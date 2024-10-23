#include <stdint.h>
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

char * blobToHexStr(uint8_t *data, int data_length) {
    char logMessage[1024];
    int offset = 0;

    // Iterate over each byte of the data and append its hex representation to logMessage
    for (int i = 0; i < data_length; i++) {
        offset += snprintf(logMessage + offset, sizeof(logMessage) - offset, "%02x", data[i]);
        if (offset >= sizeof(logMessage)) {
            break;  // Prevent buffer overflow, stop if we exceed the buffer size
        }
    }

    // Ensure the string is null-terminated
    logMessage[offset - 1] = '\0';  // Replace the last space with a null terminator
  
    return &logMessage;
}