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
#include <errno.h>
#include <time.h>
#include "event-stream.h"

void eventCallback(connectionInfo * conn, char * event)
{
  time_t ticks;
  char * receivedTimestamp;
  const char * template = "{\"deviceIp\":\"%s\",\"received\":\"%s\",\"event\":%s}\n";

  ticks = time(NULL);
  receivedTimestamp = malloc(sizeof(char) * 25);
  if (receivedTimestamp == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for timestamp: %s\n", strerror(errno));
    return;
  }

  sprintf(receivedTimestamp, "%.24s", ctime(&ticks));
  printf(template, conn->clientIp, receivedTimestamp, event);

  free(receivedTimestamp);
}