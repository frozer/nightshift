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
#ifndef SOCKET_SERVER_CONFIG_H
#define SOCKET_SERVER_CONFIG_H

#define MAX_CONN 5

typedef void (*on_message_t)(void *responsePayload, void *data, char *clientIp);

struct SocketConfig {
  unsigned int port;
  on_message_t on_message;
  unsigned int debug;
};

struct ConnectionPayload {
  int sockfd;
  char * clientIp[16];
  on_message_t on_message;
  unsigned int debug;
  unsigned short int workerId;
};

void startSocketService(struct SocketConfig * config);
void stopSocketService();

#endif // SOCKET_SERVER_CONFIG_H