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
#include "liblogger.h"

union rawMessage {
  struct {
    char aLength[3];
    uint8_t payload[BUFFERSIZE - sizeof(char) * 3];
  } data;
  uint8_t raw[BUFFERSIZE];
};

Events * dozor_unpackV2(CryptoSession * crypto, uint8_t * raw, char * pinCode)
{
  uint8_t * ptr;
  int c, n;
  int msgLength;
  unsigned short int index, dataIndex;  
  short int result;
  union rawMessage packet;
  DozorReport * deviceReport;
  EventInfo * eventInfo;

  Events *events = malloc(sizeof(Events));
  if (events == NULL) {
      fprintf(stderr, "Unable to allocate memory for events: %s\n", strerror(errno));
      return NULL; // Return NULL on memory allocation failure
  }

  if (crypto == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    events->errorCode = HANDLER_UNABLE_TO_ALLOCATE_MEMORY_CRYPTO_SESSION; 
    return events;
  }

  memcpy(&packet, raw, BUFFERSIZE);

  msgLength = strtol(packet.data.aLength, 0, 10) + 4;
  
  short int initError = initializeDozorCrypto(
    crypto,
    (const unsigned char *) pinCode,
    (const unsigned char *) packet.raw,
    msgLength - 4 // strtol(packet.data.aLength, 0, 10)
  );
  if (initError)
  {
    printf("Crypto session not initialized. Error - %d\n", initError);
    events->errorCode = initError;
    return events;
  }
  
  ptr = (uint8_t*) packet.raw;

  // Allocate memory for the hex string (2 characters per byte + 1 for null terminator)
  char *hexStr = (char *)malloc(msgLength * 2 + 1);
  if (!hexStr) {
      return NULL;  // Return NULL if memory allocation fails
  }

  blobToHexStr(hexStr, ptr, msgLength);

  logger(LOG_LEVEL_DEBUG, "libdozor", "Incoming length - %d, message length - %d, %s", msgLength, msgLength - 4, hexStr);

  free(hexStr);

  deviceReport = malloc(sizeof(DozorReport));
  if (deviceReport == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for report: %s\n", strerror(errno));
    events->errorCode = HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT;
    return events;
  }
  
  memset(deviceReport, 0, sizeof(DozorReport));

  result = getReport(deviceReport, crypto, (const unsigned char *) packet.raw, msgLength);
  if ( result < 0)
  {
    fprintf(stderr, "ERROR: Unable to recognize message!!! Error - %d\n", result);
    free(deviceReport);
    events->errorCode =  HANDLER_UNABLE_TO_RECOGNIZE_MESSAGE;
    return events;
  }

  logger(LOG_LEVEL_DEBUG, "libdozor", "Total %d events found", deviceReport->eventTotals);

  if (deviceReport->eventTotals == 0)
  {
    eventInfo = malloc(sizeof(EventInfo));
    if (eventInfo == NULL)
    {
      fprintf(stderr, "Unable to allocate memory for event info structure: %s\n", strerror(errno));
      return HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT;
    }

    memset(eventInfo, 0, sizeof(EventInfo));

    getKeepAliveEvent(eventInfo, deviceReport->site, &(deviceReport->info));
    
    logger(LOG_LEVEL_DEBUG, "libdozor", "[%d] %s", eventInfo->eventType, eventInfo->event);

    events->length = 1;
    events->errorCode = 0;
    memcpy(&(events->items[0]), eventInfo, sizeof(EventInfo));

    free(eventInfo);
    free(deviceReport);

    return events;
  }

  
  for (index = 0; index < deviceReport->eventTotals; index++)
  {
    eventInfo = malloc(sizeof(EventInfo));
    if (eventInfo == NULL)
    {
      fprintf(stderr, "Unable to allocate memory for event info structure: %s\n", strerror(errno));
      return HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT;
    }

    memset(eventInfo, 0, sizeof(EventInfo));
    
    convertDeviceEventToCommon(eventInfo, deviceReport->site, &(deviceReport->events[index]));
    
    logger(LOG_LEVEL_DEBUG, "libdozor", "[%d] %s", eventInfo->eventType, eventInfo->event);
    
    memcpy(&(events->items[index]), eventInfo, sizeof(EventInfo));

    free(eventInfo);
  }

  events->length = deviceReport->eventTotals;
  events->errorCode = 0;

  free(deviceReport);

  return events;
}

unsigned short int dozor_pack(
  CommandResponse * command, 
  CryptoSession * crypto, 
  const unsigned int commandId, 
  char * commandValue
)
{
 
  unsigned short int answerLength;
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
  response->time = time(NULL) - DATE_TIME_OFFSET;
  response->unknownOne = 0;
  response->unknownTwo = 0x7c;
  
  cmdValue = malloc(sizeof(char) * 32);
  if (cmdValue == NULL)
  {
    free(response);
    fprintf(stderr, "***libdozor.c: Unable to allocate memory for cmdValue: %s\n", strerror(errno));
    return -1;
  }

  if ((strlen(commandValue) > 1) && (commandId > 0))
  {
    sprintf(cmdValue, "%s&%08x", commandValue, __builtin_bswap32(commandId));      
    logger(LOG_LEVEL_DEBUG, "libdozor", "New command - %s", cmdValue);
  } else {
    strncpy(cmdValue, DEFAULT_ANSWER, 31);
    cmdValue[31] = '\0';
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

  snprintf(answer, answerLength, "%s!", cmdValue);
  free(cmdValue);

  logger(LOG_LEVEL_DEBUG, "libdozor", "command - %s (%d)", answer, answerLength);

  memcpy(response->encrypted, answer, sizeof(char) * answerLength);
  
  if (encrypt(response->encrypted, crypto, answerLength) != 0)
  {
    free(response);
    fprintf(stderr, "Unable to encrypt message:\"%s\" %s\n", answer, strerror(errno));
    free(answer);
    return -1;
  }
  
  memcpy(&command->response, response, sizeof(DozorResponse));
  command->responseLength = answerLength + 6;
  
  free(answer);
  logger(LOG_LEVEL_DEBUG, "libdozor", "answer freed");
  
  free(response);
  logger(LOG_LEVEL_DEBUG, "libdozor", "response freed");

  return 0;
}