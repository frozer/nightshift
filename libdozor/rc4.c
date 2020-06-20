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
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "rc4.h"

#define POOL_SIZE 256

extern unsigned short int debugMode;

void getCryptoSession(CryptoSession * crypto, const uint8_t* key)
{
  const unsigned short int key_length = 16;
  unsigned short int i = 0;
  unsigned short int j = 0;
  unsigned short int temp = 0;

  for (i = 0; i < POOL_SIZE; i++)
  {
    crypto->pool[i] = i;
  }

  for (i=0; i < POOL_SIZE; i++)
  {
    j = (j + crypto->pool[i] + key[i % 16]) % POOL_SIZE;
    swap(crypto->pool, i, j);
  }
}

/**
 * encrypt/decrypt function
 */
void codec(unsigned char* data, CryptoSession * crypto, const size_t msgLength)
{
  unsigned short int i = crypto->iterator;
  unsigned short int j = crypto->pointer;
  unsigned short int kword;
  unsigned short int data_index;
  unsigned char old;

  if (debugMode == 1)
  {
    printf("***rc4(codec): called for - %s\n", data);
    printf("***rc4(codec): iterator %d\n", i);
    printf("***rc4(codec): pointer %d\n", j);
  }

  // convert data
  for (data_index = 0; data_index < msgLength; data_index++) {    
    old = data[data_index];
    i = (i + 1) % POOL_SIZE;
    j = (j + crypto->pool[i]) % POOL_SIZE;
    swap(crypto->pool, i, j);
    kword = crypto->pool[(crypto->pool[i] + crypto->pool[j]) % POOL_SIZE];
    data[data_index] = old ^ kword;

    if (debugMode == 1)
    {
      printf("***rc4(codec): [%d] 0x%x -> 0x%x\n", data_index, old, data[data_index]);
    }
  }

  crypto->iterator = i;
  crypto->pointer = j;
}

static void swap(unsigned short int* pool, const unsigned short int src, const unsigned short int dst)
{
    unsigned short int temp = pool[src];
    pool[src] = pool[dst];
    pool[dst] = temp;
}