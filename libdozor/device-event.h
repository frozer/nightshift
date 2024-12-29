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

#ifndef DOZOR_DEVICE_EVENT_H
#define DOZOR_DEVICE_EVENT_H

#define MAX_DEVICE_EVENT_DATA_SIZE 80
#define MAX_EVENTS_PER_DEVICE 256
// 4 bytes for date, 1 byte for event type id
#define MESSAGE_ALIGN_SIZE 5

typedef struct EVENT {
  uint8_t type;
  uint32_t time;
  uint8_t dataLength;
  uint8_t data[MAX_DEVICE_EVENT_DATA_SIZE];
} DeviceEvent;

unsigned short int getDeviceEvents(const uint8_t * raw, long int bufSize, DeviceEvent * events);

#endif