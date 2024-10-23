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

#ifndef DOZOR_H
#define DOZOR_H
#include "session.h"
#include "dozor-crypto.h"
#include "event.h"

#define DEFAULT_ANSWER ""
#define END_OF_COMMAND "!"

#define HANDLER_SOCKET_READ_ERROR -1
#define HANDLER_UNABLE_TO_ALLOCATE_MEMORY_CRYPTO_SESSION -2
#define HANDLER_CRYPTO_SESSION_NOT_INITIALIZED -4
#define HANDLER_UNABLE_TO_ALLOCATE_MEMORY_REPORT -16
#define HANDLER_UNABLE_TO_RECOGNIZE_MESSAGE -32

#define BUFFERSIZE 1024

typedef struct {
  int sock;
  char clientIp[16];
  unsigned char pinCode[32];
  unsigned short int debug;
} connectionInfo;

typedef struct {
  DozorResponse response;
  unsigned short int responseLength;
} CommandResponse;

/* decrypt and unpack device messages from raw data packet */
Events * dozor_unpackV2(CryptoSession * crypto, uint8_t * raw, char * pinCode);

/* encrypt command for device */
unsigned short int dozor_pack(CommandResponse * , CryptoSession *, unsigned int commandId, char * commandValue);


#endif