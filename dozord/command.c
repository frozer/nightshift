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

short int getNextCommand(Commands * commands)
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

void readCommandsFromFile(Commands * commands, char * fn, const unsigned short int debugMode)
{
  int ch;
  FILE * fp;
  char chunk[32];
  int i = 0;
  
  fp = fopen(fn, "r");

  if ( fp == NULL)
  {
    fp = fopen(fn, "w");

    if(fp == NULL)
    {
        fprintf(stderr, "%s %s\n", strerror(errno), fn);
        return;
    }
    fclose(fp);
    fp = fopen(fn, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "%s %s\n", strerror(errno), fn);
        return;
    }
  }
  
  while (fgets(chunk, sizeof(chunk), fp) != NULL)
  {
    if (strlen(chunk) > 1)
    {
      strncpy(commands->items[i].value, chunk, strlen(chunk) - 1);
      
      if (debugMode) {
        printf("***commands.c: new command - %s\n", commands->items[i].value);
      }

      commands->items[i].done = 0;
      commands->items[i].id = i + 1;

      i++;
    }
  }

  commands->length = i;

  if (fclose(fp) != 0)
  {
    fprintf(stderr, "Close commands file failed: %s\n", strerror(errno));
    exit(-1);
  }
}