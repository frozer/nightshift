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
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <dozor.h>
#include "socket-server.h"
#include "logger.h"

pthread_t ServiceThreadId;
pthread_t connectionWorkers[5] = {0};
unsigned int socketExitRequested = 0;

void * connectionCb(void * args) {
  struct ConnectionPayload * payload = (struct ConnectionPayload *) args;
  uint8_t packet[BUFFERSIZE];
  char logMessage[2048];

  int sockfd = payload->sockfd; 

  CryptoSession * crypto = malloc(sizeof(CryptoSession));
  if (crypto == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    close(sockfd);
    pthread_exit(0);
    return 0; 
  }

  int c = read(sockfd, &packet, BUFFERSIZE);
  if (c < 0) {
    free(crypto);
    fprintf(stderr, "ERROR reading from socket: %s\n", strerror(errno));
    close(sockfd);
    pthread_exit(0);
    return 0;
  }

  snprintf(logMessage, sizeof(logMessage), "TCP::%s %s", payload->clientIp, (char *) packet);
  logger(LOG_LEVEL_INFO, logMessage);   
  
  free(crypto);
  close(sockfd);

  connectionWorkers[payload->workerId] = 0;

  snprintf(logMessage, sizeof(logMessage), "TCP::%s closed", payload->clientIp);
  logger(LOG_LEVEL_INFO, logMessage);   

  
  pthread_exit(0);
  
  return NULL;
  
  // int res = dozor_unpack(crypto, payload->pinCode, packet, payload->on_message, payload->debug);
  // if (res < 0) {
  //   free(crypto);
  //   close(sockfd);
  //   pthread_exit(0);
  //   return 0;
  // }
}

void * startSocketListener(void * args) {
  struct SocketConfig * socketConfig = (struct SocketConfig *) args;

  int sockfd, newsockfd, pid, rc; 
  int port;
  char clientIp[INET_ADDRSTRLEN];
	struct sockaddr_in servaddr;
  struct sockaddr_in cli;
  char logMessage[256];
  struct ConnectionPayload infos[5];
  socklen_t len;

  unsigned short int i = 0;

  // socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		fprintf(stderr, "Socket create failed: %s\n", strerror(errno));
		exit(-1); 
	}

  // set sock options
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1)
  {
    fprintf(stderr, "Socket options setup failed: %s\n", strerror(errno));
    exit(-1);
  }

	bzero(&servaddr, sizeof(servaddr)); 
  
	// assign IP, PORT
	servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(socketConfig->port); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (struct sockaddr *) &servaddr, (socklen_t) sizeof(servaddr))) != 0) { 
		fprintf(stderr, "Socket bind failed: %s\n", strerror(errno));
		exit(-1); 
	}

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		fprintf(stderr, "Socket listen failed: %s\n", strerror(errno));
		exit(-1); 
	}

  snprintf(logMessage, sizeof(logMessage), "TCP::Listen *:%d", socketConfig->port);
  logger(LOG_LEVEL_INFO, logMessage);

  while(!socketExitRequested)
  {
    if (i < 5) {
      if (!connectionWorkers[i]) {

        struct ConnectionPayload payload;

        // Accept the data packet from client and verification
        len = sizeof(cli);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli, &len);

        if (newsockfd < 0) { 
          fprintf(stderr, "Server accept failed: %s\n", strerror(errno));
          pthread_exit(NULL);
          exit(-1);
        }

        inet_ntop( AF_INET, &cli.sin_addr, clientIp, INET_ADDRSTRLEN );   

        strncpy(payload.clientIp, clientIp, sizeof(clientIp));
        strncpy(payload.pinCode, "", sizeof(socketConfig->pinCode));
        payload.siteId = socketConfig->siteId;
        payload.sockfd = newsockfd;
        payload.debug = socketConfig->debug;
        payload.on_message = socketConfig->on_message;
        payload.workerId = i;

        if (pthread_create(&connectionWorkers[i], NULL, connectionCb, (void *) &payload) != 0) {
          connectionWorkers[i] = 0;
          fprintf(stderr, "Connection workers init failed: %s\n", strerror(errno));
          exit(-1);
        }
      }
      i++;
    
    } else {
      i = 0;
    }
  }

  logger(LOG_LEVEL_INFO, "TCP::Closing client connections...");
  for (i = 0; i < 5; i++) {
    if (connectionWorkers[i]) {
      pthread_join(connectionWorkers[i], NULL);
      connectionWorkers[i] = 0;
    }
  }
  logger(LOG_LEVEL_INFO, "TCP::Client connections closed.");

  close(sockfd);

  pthread_exit(0);

  return NULL;
}

// // handle network connections
// void* handle(void* arg) 
// {
//   int c;
//   connectionInfo * conn = (connectionInfo*) arg;
//   uint8_t packet[BUFFERSIZE];
//   int sockfd = conn->sock;

//   CryptoSession * crypto = malloc(sizeof(CryptoSession));
//   if (crypto == NULL)
//   {
//     fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
//     close(sockfd);
//     return 0; 
//   }

//   c = read(sockfd, &packet, BUFFERSIZE);
//   if (c < 0) {
//     fprintf(stderr, "ERROR reading from socket: %s\n", strerror(errno));
//     free(crypto);
//     close(sockfd);
//     return 0;
//   }

//   int res = dozor_unpack(crypto, conn, packet, &eventCallback, conn->debug);
//   if (res < 0) {
//     free(crypto);
//     close(sockfd);
//     return 0;
//   }
  
//   pthread_mutex_lock(&writelock);

//   short int commandId = answerDevice(sockfd, crypto, commands, conn->debug);

//   if (commands != NULL) {
//     if (commandId != -1 && commandId < commands->length)
//     {
//       commands->items[commandId].done = 1;  
//     }
//   }
//   pthread_mutex_unlock(&writelock);

//   close(sockfd);

//   free(crypto);
//   return (void *) 0;
// }


void startSocketService(struct SocketConfig * config) {

  if (pthread_create(&ServiceThreadId, NULL, startSocketListener, config) != 0)
    ServiceThreadId = 0;
}

void stopSocketService() {
  socketExitRequested = 1;

  if (ServiceThreadId) {
    pthread_join(ServiceThreadId, NULL);
    ServiceThreadId = 0;
  }
}