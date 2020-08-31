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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "device-event.h"

extern unsigned short int debugMode;

const unsigned char MSGDATASIZE[68] = { 
  0, 2, 2, 1, 1,
  1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1,
  1, 1, 4, 0, 0,
  0, 1, 1, 0, 0,
  0, 1, 1, 1, 1,
  1, 1, 1, 0, 2,
  2, 4, 4, 0, 16,
  1, 0, 17, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 4, 4, 1,
  1, 1, 1, 5, 3,
  37, 4, 73 };

unsigned short int getDeviceEvents(const uint8_t * raw, long int bufSize, DeviceEvent events[])
{
  long int totalLength = bufSize;
  unsigned short int index = 0; 
  unsigned short int eventSize = 0;
  unsigned short int eventCount = 0;
  unsigned short int dataIndex = 0;

  DeviceEvent * deviceEvent = malloc(sizeof(DeviceEvent));

  if (deviceEvent == NULL)
  {
    fprintf(stderr, "***device-event.c: Unable to allocate memory for device event: %s\n", strerror(errno));
    return 64;
  }

  if (bufSize == 0)
  {
    if (debugMode == 1)
    {
      printf("***device-event(getDeviceEvents): No events\n");
    }
    free(deviceEvent);
    return 0;
  }

  while (totalLength > 0)
  {
    eventSize = MSGDATASIZE[raw[index]] + MESSAGE_ALIGN_SIZE;

    getDeviceEvent(deviceEvent, &raw[index], eventSize);
    memcpy(&events[eventCount], deviceEvent, sizeof(DeviceEvent));

    eventCount += 1;
    index = index + eventSize;
    totalLength = totalLength - eventSize;
  }

  free(deviceEvent);
  return eventCount;
}

void getDeviceEvent(DeviceEvent * deviceEvent, const uint8_t * raw, unsigned short int eventSize)
{
  unsigned short int eventDataSize = 0;
  
  eventDataSize = eventSize - sizeof(deviceEvent->time);
  
  deviceEvent->type = raw[0];
  deviceEvent->dataLength = eventDataSize;

  memcpy(&deviceEvent->time, &raw[5 + eventSize - sizeof(deviceEvent->time)], sizeof(deviceEvent->time));
  memcpy(&deviceEvent->data, &raw[5], (size_t) eventDataSize);
}