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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dozor.h>
#include "./command.h"

#define DEFAULT_ANSWER ""

short int answerDevice(int sockfd, CryptoSession * crypto, Commands * commands, unsigned short int debugMode)
{
  short int res = -1;
  short int index = 0;
  
  if (commands == NULL)
  {
    return -1;
  }

  CommandResponse * response;
  
  short int found = getNextCommand(commands);

  response = malloc(sizeof(CommandResponse));
  if (response == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for response: %s\n", strerror(errno));
    return -1;
  }

  if (found != -1)
  {
    if ((commands->items[found].done != 1) && (strlen(commands->items[found].value) > 1))
    {
      res = dozor_pack(response, crypto, commands->items[found].id, commands->items[found].value, debugMode);
    } else {
      res = dozor_pack(response, crypto, 1, DEFAULT_ANSWER, debugMode);
    }
  } else {
    res = dozor_pack(response, crypto, 1, DEFAULT_ANSWER, debugMode);
  }

  if (response == NULL || res == -1)
  {
    free(response);
    fprintf(stderr, "Unable to encrypt command!\n");
    return -1;
  }

  int n = 0;
  uint8_t * ptr = (uint8_t*) response;
  int written = 0;
  int toWrite = response->responseLength;

  if (debugMode)
  {
    printf("\nResponse message: ");
    for (index = 0; index < toWrite; index++)
    {
      printf("%02X", *(ptr + index));
    }
    printf("\n");
  }

  while(response->responseLength > written)
  {
    n = send(sockfd, ptr, (toWrite - written), 0x4000);
    if (n < 0) {
      free(response);
      fprintf(stderr, "Socket send failed: %s\n", strerror(errno));
      return -1;
    }
    ptr += 1;
    written += n;
  }

  free(response);

  return found;
}