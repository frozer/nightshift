/*
  This file is part of NightShift Message Library.

  NightShift Message Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NightShift Message Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NightShift Message Library. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../liblogger/liblogger.h"

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

void prettyLogger(LogLevel level, const char* source, const char* message) {
    char dateTimeBuffer[30];  // Buffer to hold the timestamp in ISO 8601 format
    getISODateTime(dateTimeBuffer, sizeof(dateTimeBuffer));

    // Concatenate timestamp with the log message
    char * logMessage = getLogMessage(level, "[%s] %s [%s] %s", dateTimeBuffer, level, source, message);

    if (!logMessage) {
      return;
    }
    
    // Print the log message
    printf("%s\n", logMessage);
}