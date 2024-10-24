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
    // Allocate memory for the hex string (2 characters per byte + 1 for null terminator)
    char *hexStr = (char *)malloc(data_length * 2 + 1);
    if (!hexStr) {
        return NULL;  // Return NULL if memory allocation fails
    }

    int offset = 0;
    for (int i = 0; i < data_length; i++) {
        offset += snprintf(hexStr + offset, 3, "%02x", data[i]);
    }

    // Ensure the string is null-terminated
    hexStr[offset] = '\0';

    return hexStr;
}

// Logger function
char * getLogMessage(LogLevel level, const char* format, ...) {
    if (level < currentLogLevel) {
        return NULL;
    }
    
    char *logMessage = (char *)malloc(sizeof(char) * 1024);
    if (!logMessage) {
        return NULL;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(logMessage, sizeof(logMessage), format, args);
    va_end(args);

    return logMessage;
}