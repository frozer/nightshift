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

struct SocketConfig {
  unsigned int siteId;
  unsigned int port;
  unsigned int debug;
  char pinCode[32];
  void * on_message;
};

struct ConnectionPayload {
  int sockfd;
  char clientIp[16];
  unsigned int siteId;
  char pinCode[32];
  void * on_message;
  unsigned int debug;
  unsigned short int workerId;
};

void startSocketService(struct SocketConfig * config);
void stopSocketService();