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

#include <inttypes.h>
#ifndef DOZOR_UTILS_H
#define DOZOR_UTILS_H
#define DATE_TIME_OFFSET 0x3A4FC880

void char2utf8(wchar_t* dest, const unsigned char* src);
char * getDateTime(const uint32_t t);

#endif