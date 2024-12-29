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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include "utils.h"

void char2utf8(wchar_t* dest, const unsigned char* src)
{
  short int new_length = strlen(src) + 1;
  short int index;

  mbstowcs(dest, src, new_length);
}

char * getDateTime(const uint32_t t) {
    time_t ot = t + DATE_TIME_OFFSET;
    struct tm *date = localtime(&ot);
    char *buffer = malloc(26);  // Enough space for the formatted string
    if (buffer != NULL) {
        strftime(buffer, 26, "%a %b %d %H:%M:%S %Y", date);
    }
    return buffer;
}
