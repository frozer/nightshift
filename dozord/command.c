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
#include <errno.h>
#include <string.h>
#include "./command.h"

short int getNextCommandIdx(Commands * commands)
{
  unsigned short int index;
  short int found = -1;

  if (commands->length == 0)
  {
    return found;
  }

  for (index = 0; index < commands->length; index++)
  {
    if (commands->items[index].done == 0)
    {
      found = index;
      break;
    }
  }

  return found;
}

void readCommandsFromString(Commands * commands, char * command)
{
  int i = commands->length;
  if (i >= MAX_COMMAND_QUEUE_LENGTH)
  {
    i = 0;
  }

  commands->length = i + 1;
  
  sprintf(commands->items[i].value, "%s", command);
  commands->items[i].done = 0;
  commands->items[i].id = i + 1;
}