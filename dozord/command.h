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

#include <inttypes.h>

#ifndef COMMAND_H
#define COMMAND_H
#define MAX_COMMAND_QUEUE_LENGTH 256

typedef struct command {
  uint32_t id;
  unsigned short int done;
  char value[32];
} Command;

typedef struct {
  Command items[MAX_COMMAND_QUEUE_LENGTH];
  unsigned short int length;
} Commands;

short int getNextCommand(Commands *);
void readCommandsFromFile(Commands *, char * fn, const unsigned short int debugMode);
void readCommandsFromString(Commands * commands, char * command, const unsigned short int debugMode);

#endif
