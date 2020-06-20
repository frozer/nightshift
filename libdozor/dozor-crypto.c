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
#include <inttypes.h>
#include "dozor-crypto.h"
#include "utils.h"
#include "rc4.h"

extern unsigned short int debugMode = 0;

short int initializeDozorCrypto(CryptoSession * crypto, 
  const unsigned char* userKey, const unsigned char * data, 
  const size_t dataLength,
  const unsigned short int masterDebugMode)
{
  wchar_t key[strlen(userKey)];
  union PKEY seededKey;
  uint32_t seed;
  uint8_t site;
  unsigned short int index;

  debugMode = masterDebugMode;

  if (crypto == NULL) {
    return ERROR_CRYPTO_SESSION_NOT_INITIALIZED;    
  }

  crypto->iterator = 0;
  crypto->pointer = 0;

  if (dataLength < SEED + 2) {
    if (debugMode == 1)
    {
      printf("***dozor-crypto(initializeDozorCrypto) ERROR: message too short - %ld!\n", dataLength);
    }
    return ERROR_MESSAGE_TOO_SHORT;
  }

  // convert string key to wchar[]
  char2utf8(key, userKey);
  
  memcpy(&seed, &data[SEED], sizeof(uint32_t));
  memcpy(&site, &data[SITE], sizeof(uint8_t));
  
  // enrich key with seed and site
  seededKey = getSeededKey(key, seed, site);

  getCryptoSession(crypto, (unsigned char*) &(seededKey.z));

  if (debugMode == 1)
  {
    printf("\n***dozor-crypto(initializeDozorCrypto) Enriched key ***\n");
    for (index = 0; index < 4; index++)
    {
      printf("[%d]: 0x%x\n", index, seededKey.y[index]);
    }
  }
  return 0;
}

short int encrypt(unsigned char* data, CryptoSession * crypto, const size_t dataLength)
{
  if (crypto == NULL)
  {
    return ERROR_CRYPTO_SESSION_NOT_INITIALIZED;    
  }

  codec(data, crypto, dataLength);
  return 0;
}

short int getReport(DozorReport * report, CryptoSession * crypto, const unsigned char * data, const size_t dataLength)
{
  DozorMessage message;
  DeviceEvent events[MAX_EVENTS_PER_DEVICE];
  unsigned short int index;
  long int actualDataLength = 0;
  long int closedLength = 0;

  if (crypto == NULL)
  {
    return ERROR_CRYPTO_SESSION_NOT_INITIALIZED;
  }

  // convert data to struct
  memcpy(&message.opened, &data[0], sizeof(message.opened));
  memcpy(&message.opened.seed, &data[SEED], sizeof(message.opened.seed));
  memcpy(&message.closed.raw, &data[CLOSED_START], sizeof(message.closed.raw));

  actualDataLength = strtol(message.opened.alength, 0, 10);
  closedLength = actualDataLength - SEED;

  if (dataLength - DEVICE_MESSAGE_LENGTH != actualDataLength)
  {
    if (debugMode == 1)
    {
      printf("***dozor-crypto(getReport) ERROR: message length not equal calculated length (%ld != %d)\n", actualDataLength, dataLength - DEVICE_MESSAGE_LENGTH != actualDataLength);
    }

    return ERROR_LENGTH_MISMATCH;
  }

  if (message.opened.magic != MAGIC_MESSAGE_START_FLAG)
  {
    if (debugMode == 1)
    {
      printf("***dozor-crypto(getReport) ERROR: message magic number not exists\n");
    }
    
    return ERROR_NO_START_FLAG;
  }
  
  if (debugMode == 1)
  {
    printf("***dozor-crypto(getReport) New Dozor Message ***\n");
    printf("Total message length - %ld\n", dataLength);
    printf("Length from message - %ld\n", actualDataLength);    
    printf("Magic field - %d\n", message.opened.magic);
    printf("Site - %d\n", message.opened.site);
    printf("Seed - 0x%x\n", message.opened.seed);
  }

  codec((unsigned char*) &message.closed, crypto, closedLength);

  // check is decryption correct - last two bytes should be 0x21
  closedLength = closedLength - 2;

  if (debugMode == 1)
  {
    for (index = 0; index < closedLength; index++)
    {
      printf("***dozor-crypto(getReport) [%d]: 0x%x\n", index, message.closed.raw[index]);
    }
  }

  if (message.closed.raw[closedLength] != MAGIC_DECRYPTED_TAILEND)
  {
    if (debugMode == 1)
    {
      printf("***dozor-crypto(getReport) ERROR: MAGIC_DECRYPTED_TAILEND #1 check %#x != %#x at position %ld\n", message.closed.raw[closedLength], MAGIC_DECRYPTED_TAILEND, closedLength);
    }
    return ERROR_DECRYPT_FAIL;
  }

  if (message.closed.raw[closedLength + 1] != MAGIC_DECRYPTED_TAILEND)
  {
    if (debugMode == 1)
    {
      printf("***dozor-crypto(getReport) ERROR: MAGIC_DECRYPTED_TAILEND #2 check %#x != %#x at position %ld\n", message.closed.raw[closedLength + 1], MAGIC_DECRYPTED_TAILEND, closedLength + 1);
    }
    return ERROR_DECRYPT_FAIL;
  }

  report->eventTotals = getDeviceEvents(message.closed.messages, closedLength - 6, events);
  report->site = message.opened.site;

  memcpy(&(report->info), &message.closed.info, sizeof(message.closed.info));
  memcpy(&(report->events), &events, sizeof(DeviceEvent) * report->eventTotals);

  return report->eventTotals;
}

union PKEY getSeededKey(const wchar_t* user_key, const uint32_t seed, const uint16_t site)
{
  union PKEY preKey;
  unsigned short index;
  uint32_t x = seed >> 8;
  uint32_t s2 = seed >> 16;
  uint32_t s3 = seed >> 24;

  preKey.y[0] = seed;
  preKey.y[1] = user_key[0] | (uint32_t)user_key[1] << 8 | (uint32_t)user_key[2] << 16 | (uint32_t)user_key[3] << 24;
  preKey.y[2] = user_key[4] | (uint32_t)user_key[5] << 8 | (uint32_t)user_key[6] << 16 | (uint32_t)user_key[7] << 24;
  preKey.y[3] = (uint16_t) site;
  preKey.z[14] = (uint8_t) seed | x;
  preKey.z[15] = (uint8_t) s2 & s3;

  return preKey;
}