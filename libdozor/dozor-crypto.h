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

#ifndef DEVICE_CRYPTO_H
#define DEVICE_CRYPTO_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "device-event.h"
#include "session.h"

// site position
#define SITE 4
// seed position
#define SEED 6
// closed part position
#define CLOSED_START 10
#define DEVICE_MESSAGE_LENGTH 4
#define MAGIC_MESSAGE_START_FLAG 0x0d
#define MAGIC_DECRYPTED_TAILEND 0x21

#define ERROR_LENGTH_MISMATCH -1
#define ERROR_NO_START_FLAG -2
#define ERROR_DECRYPT_FAIL -3
#define ERROR_MESSAGE_TOO_SHORT -4
#define ERROR_CRYPTO_SESSION_NOT_INITIALIZED -5

typedef struct DEVICE_INFO {
  uint8_t tag;
  short unsigned int channel:4;
  short unsigned int sim: 4;
  uint8_t voltage;
  short unsigned int gsm_signal: 6;
  short unsigned int extra_index: 2;
  uint8_t extra_value;
} DeviceInfo;

typedef struct DOZOR_MESSAGE {
    struct {
        char alength[3];
        char magic;
        uint16_t site;
        uint32_t seed;
    } opened;
    union {
      struct {
        DeviceInfo info;
        uint8_t messages[1];
      };
        // FIXME: absolutely random big number :-)
        uint8_t raw[1024];
    } closed;
} DozorMessage;

typedef struct DOZOR_REPORT {
  uint8_t eventTotals;
  DeviceEvent events[256];
  DeviceInfo info;
  uint16_t site;
} DozorReport;

typedef struct DOZOR_RESPONSE {
  uint32_t time;
  uint8_t unknownOne;
  uint8_t unknownTwo;
  unsigned char encrypted[124];
} DozorResponse;
union PKEY
{
  uint64_t x[2];
  uint32_t y[4];
  uint8_t z[16];
};

// Initialize connection state
short int initializeDozorCrypto(CryptoSession * crypto,
  const unsigned char* userKey, const unsigned char * data, 
  const size_t dataLength,
  const unsigned short int masterDebugMode);

// Return encrypted data
short int encrypt(unsigned char* data, CryptoSession * crypto, const size_t dataLength);

// Return decrypted device report
short int getReport(DozorReport * report, CryptoSession * crypto, const unsigned char * data, const size_t dataLength);

union PKEY getSeededKey(const wchar_t* key, const uint32_t seed, const uint16_t site);

#endif