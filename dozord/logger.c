#include <string.h>
#include <time.h>
#include <sys/time.h>

void getISODateTime(char* buffer, size_t bufferSize) {
    // Get current time
    struct timeval tv;
    gettimeofday(&tv, NULL);  // Get time including microseconds

    time_t t = tv.tv_sec;
    struct tm tm = *localtime(&t);  // Convert to local time

    // Get timezone offset in hours and minutes
    char timezoneSign = (tm.tm_gmtoff < 0) ? '-' : '+';
    int timezoneHours = (int)(tm.tm_gmtoff / 3600);
    int timezoneMinutes = (int)((tm.tm_gmtoff % 3600) / 60);

    // Format the date and time as YYYY-MM-DDTHH:MM:SS.sss±hh:mm
    snprintf(buffer, bufferSize, 
             "%04d-%02d-%02dT%02d:%02d:%02d.%03ld%c%02d:%02d",
             tm.tm_year + 1900, 
             tm.tm_mon + 1, 
             tm.tm_mday,
             tm.tm_hour, 
             tm.tm_min, 
             tm.tm_sec,
             tv.tv_usec / 1000,  // Convert microseconds to milliseconds
             timezoneSign, 
             timezoneHours, 
             timezoneMinutes);
}

void logger(const char* level, const char* message) {
    char dateTimeBuffer[30];  // Buffer to hold the timestamp in ISO 8601 format
    getISODateTime(dateTimeBuffer, sizeof(dateTimeBuffer));

    // Concatenate timestamp with the log message
    char logMessage[256];  // Buffer for the final log message
    snprintf(logMessage, sizeof(logMessage), "[%s] %s %s", dateTimeBuffer, level, message);

    // Print the log message
    printf("%s\n", logMessage);
}