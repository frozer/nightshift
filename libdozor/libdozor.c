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
#include <errno.h>
#include <time.h> 
#include "dozor-crypto.h"
#include "utils.h"
#include "event.h"
#include "dozor.h"

union rawMessage {
  struct {
    char aLength[3];
    uint8_t payload[BUFFERSIZE - sizeof(char) * 3];
  } data;
  uint8_t raw[BUFFERSIZE];
};

int dozor_unpack(CryptoSession * crypto, connectionInfo * conn, 
  uint8_t * raw,
  void (* callback)(),
  const unsigned short int debugMode)
{
  char * pinCode = conn->pinCode;

  uint8_t * ptr;
  int c, n;
  int msgLength;
  unsigned short int index, dataIndex;  
  short int result;
  union rawMessage packet;
  DozorReport * report;
  EventInfo * eventInfo;
  
  if (crypto == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    return HANDLER_UNABLE_TO_ALLOCATE_MEMORY_CRYPTO_SESSION; 
  }
  memcpy(&packet, raw, BUFFERSIZE);

  msgLength = strtol(packet.data.aLength, 0, 10) + 4;
  
  short int initialized = initializeDozorCrypto(crypto, (const unsigned char *) pinCode, (const unsigned char *) packet.raw, msgLength - 4, debugMode);
  if (initialized)
  {
    printf("Crypto session not initialized. Error - %d\n", initialized);
    return HANDLER_CRYPTO_SESSION_NOT_INITIALIZED;
  }
  
  ptr = (uint8_t*) packet.raw;

  if (debugMode)
  {
    printf("\n>>> Incoming message...\n");
    printf("[%d]: ", msgLength - 4);
    for (index = 0; index < msgLength; index++)
    {
      printf("%02X", *(ptr + index));
    }
    printf("\n");
    printf(">>> Incoming message End\n");
  }

  report = malloc(sizeof(DozorReport));
  if (report == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for report: %s\n", strerror(errno));
    return HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT;
  }

  result = getReport(report, crypto, (const unsigned char *) packet.raw, msgLength);
  if ( result < 0)
  {
    fprintf(stderr, "ERROR: Unable to recognize message!!! Error - %d\n", result);
    free(report);
    return HANDLER_UNABLE_TO_RECOGNIZE_MESSAGE;
  }

  if (debugMode)
  {
    printf(">>> Total %d events found\n", report->eventTotals);
  }

  if (report->eventTotals == 0)
  {
    eventInfo = malloc(sizeof(EventInfo));
    if (eventInfo == NULL)
    {
      fprintf(stderr, "Unable to allocate memory for event info structure: %s\n", strerror(errno));
      return HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT;
    }
    getKeepAliveEvent(eventInfo, report->site, &(report->info));
    
    if (debugMode)
    {
      printf("[%d] %s\n", eventInfo->eventType, eventInfo->event);
    }

    callback(conn, eventInfo);

    free(eventInfo);
  }

  for (index = 0; index < report->eventTotals; index++)
  {
    eventInfo = malloc(sizeof(EventInfo));
    if (eventInfo == NULL)
    {
      fprintf(stderr, "Unable to allocate memory for event info structure: %s\n", strerror(errno));
      return HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT;
    }

    convertDeviceEventToCommon(eventInfo, report->site, &(report->events[index]));
    
    if (debugMode) {
      printf("[%d] %s\n", eventInfo->eventType, eventInfo->event);
    }
    callback(conn, eventInfo);

    free(eventInfo);
  }
  
  free(report);

  return 0;
}

unsigned short int dozor_pack(CommandResponse * command, 
  CryptoSession * crypto, unsigned int commandId, char * commandValue,
  const unsigned short int debugMode)
{
  int n;
  unsigned short int answerLength, index;
  uint8_t * ptr;
  char * answer;
  char * cmdValue;

  if (crypto == NULL)
  {
    return -1;
  }

  DozorResponse * response = malloc(sizeof(DozorResponse));
  if (response == NULL)
  {
    fprintf(stderr, "***libdozor.c: Unable to allocate memory for response: %s\n", strerror(errno));
    return -1;
  }

  cmdValue = malloc(sizeof(char) * 32);
  if (cmdValue == NULL)
  {
    free(response);
    fprintf(stderr, "***libdozor.c: Unable to allocate memory for cmdValue: %s\n", strerror(errno));
    return -1;
  }

  response->time = time(NULL) - DATE_TIME_OFFSET;
  response->unknownOne = 0;
  response->unknownTwo = 0x7c;

  if ((strlen(commandValue) > 1) && (commandId > 0))
  {      
      sprintf(cmdValue, "%s&%08x", commandValue, __builtin_bswap32(commandId));
      if (debugMode)
      {
        printf("***libdozor.c: New command - %s\n", cmdValue);
      }
  } else {
    strcpy(cmdValue, DEFAULT_ANSWER);
  }

  answerLength = strlen(cmdValue) + 2;
  answer = malloc(answerLength);

  if (answer == NULL)
  {
    free(cmdValue);
    free(response);
    fprintf(stderr, "***libdozor.c: Unable to allocate memory for command: %s\n", strerror(errno));
    return -1;
  }

  memcpy(answer, cmdValue, answerLength);

  strcat(answer, "!");
  
  if (debugMode)
  {
    printf("***libdozor.c: command - %s (%d)\n", answer, answerLength);
  }

  memcpy(response->encrypted, answer, sizeof(char) * answerLength);
  
  if (encrypt(response->encrypted, crypto, answerLength) != 0)
  {
    free(cmdValue);
    free(response);
    fprintf(stderr, "Unable to encrypt message:\"%s\" %s\n", answer, strerror(errno));
    free(answer);
    return -1;
  }
  
  if (debugMode)
  {
    ptr = (uint8_t*) response->encrypted;
    printf("\n***libdozor.c: encrypted response: ");
    for (index = 0; index < answerLength; index++)
    {
      printf("%02X", *(ptr + index));
    }
    printf("\n");

    ptr = (uint8_t*) response;
    printf("\n***libdozor.c: response: ");
    for (index = 0; index < answerLength + 6; index++)
    {
      printf("%02X", *(ptr + index));
    }
    printf("\n");
  }

  memcpy(&command->response, response, sizeof(DozorResponse));
  command->responseLength = answerLength + 6;
  
  free(cmdValue);
  free(answer);
  free(response);

  return 0;
}