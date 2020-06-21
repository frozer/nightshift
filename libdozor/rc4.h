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

#ifndef RC4_H
#define RC4_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "session.h"

void codec(unsigned char* data, CryptoSession * crypto, 
  const size_t msgLength);
void getCryptoSession(CryptoSession * crypto, const uint8_t* key);
static void swap(unsigned short int* pool, 
  const unsigned short int src, const unsigned short int dst);

#endif