#ifndef LIBLOGGER_H
#define LIBLOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
// Log levels
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

// Function prototypes
void set_log_level(LogLevel level);
LogLevel get_log_level();
char * logLevel2Str(LogLevel level);
void logger(LogLevel level, const char* module, const char* format, ...);
void blobToHexStr(char *res, uint8_t *data, int data_length);
#endif // LIBLOGGER_H
