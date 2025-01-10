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
#include "liblogger.h"
#include "device-event.h"

static const unsigned char MSGDATASIZE[68] = {
  // 0 - 4 
  0, 2, 2, 1, 1,
  // 5 - 9
  1, 1, 1, 1, 1, 
  // 10 - 14
  1, 1, 1, 1, 1,
  // 15 - 19
  1, 1, 4, 0, 0,
  // 20 - 24
  0, 1, 1, 0, 0,
  // 25 - 29
  0, 1, 1, 1, 1,
  // 30 - 34
  1, 1, 1, 0, 2,
  // 35 - 39
  2, 4, 4, 0, 16,
  // 40 - 44
  1, 0, 17, 1, 1,
  // 45 - 49
  1, 1, 1, 1, 1,
  // 50 - 54
  1, 1, 1, 1, 1,
  // 55 - 59
  1, 1, 4, 4, 1,
  // 60 - 64
  1, 1, 1, 5, 3,
  // 65 - 68
  37, 4, 73 };

static void getDeviceEvent(DeviceEvent * deviceEvent, const uint8_t * raw, unsigned short int eventDataSize)
{
  deviceEvent->type = raw[0];
  deviceEvent->dataLength = eventDataSize;

  memcpy(&deviceEvent->time, &raw[1], sizeof(deviceEvent->time));
  memcpy(&deviceEvent->data, &raw[MESSAGE_ALIGN_SIZE], (size_t) eventDataSize);
}

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
    return 0;
  }

  memset(deviceEvent, 0, sizeof(DeviceEvent));
  
  if (bufSize == 0)
  {
    logger(LOG_LEVEL_DEBUG, "device-event(getDeviceEvents)", "No events\n");
    
    free(deviceEvent);
    return 0;
  }

  while (totalLength > 0)
  {
    eventSize = MSGDATASIZE[raw[index]] + MESSAGE_ALIGN_SIZE;

    char *hexStr = (char *)malloc(eventSize * 2 + 1);
    if (hexStr) {
        
      blobToHexStr(hexStr, &raw[index], eventSize);

      logger(LOG_LEVEL_DEBUG, "device-event(getDeviceEvents)", "[%d] event id 0x%x: %s", eventCount, raw[index], hexStr);

      free(hexStr);
    }
    
    getDeviceEvent(deviceEvent, &raw[index], MSGDATASIZE[raw[index]]);
    memcpy(&events[eventCount], deviceEvent, sizeof(DeviceEvent));

    eventCount += 1;
    index = index + eventSize;
    totalLength = totalLength - eventSize;
  }

  free(deviceEvent);
  return eventCount;
}
