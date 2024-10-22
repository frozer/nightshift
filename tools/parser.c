/*
  This file is part of NightShift.

  NightShift is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NightShift is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NightShift. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dozor.h>

#define BUFFERSIZE 1024

union rawMessage {
  struct {
    char aLength[3];
    uint8_t payload[BUFFERSIZE - sizeof(char) * 3];
  } data;
  uint8_t raw[BUFFERSIZE];
};

void hexstr2char(char * dest, const char *hexstr, const size_t len)
{
  int j;
  size_t i;

  for (i=0, j=0; j<len; i+=2, j++)
      dest[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i+1] % 32 + 9) % 25;
  
  dest[len] = '\0';
}

int main(int argc, char **argv) 
{
  unsigned char *data;
  char logMessage[2048];

  size_t msgLength;
  CryptoSession * crypto;
  union rawMessage packet;

  if (argc < 3)
  {
    printf("./parser <key> <message>\n");
    return 0;
  }
  
  msgLength = strlen(argv[2]);
  if (msgLength % 2 != 0)
  {
    fprintf(stderr, "Message length not odd\n");
    return -1;
  }

  data = malloc(sizeof(char) * BUFFERSIZE);
  if (data == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for data - %s\n", strerror(errno));
    return -1;
  }

  hexstr2char(data, argv[2], msgLength / 2);

  memcpy(&packet, data, sizeof(char) * msgLength / 2);

  crypto = malloc(sizeof(CryptoSession));
  if (crypto == NULL)
  {
    free(data);
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    return -1;
  }
  
  Events * events = dozor_unpackV2(crypto, data, argv[1], 1);
  if (events != NULL && events->errorCode == 0) {
    for (int index = 0; index < events->length; index++) {
      snprintf(logMessage, sizeof(logMessage), "%s", events->items[index].event);
      printf("%s\n", logMessage);
    }

    free(data);
    free(crypto);
    return -1;
  }

  free(data);
  free(crypto);
}