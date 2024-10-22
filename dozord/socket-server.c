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
pthread_t connectionWorkers[MAX_CONN] = {0};
unsigned int socketExitRequested = 0;
pthread_mutex_t GlobaSocketLock;

void log_incoming_data(const char *clientIp, uint8_t *data, int data_length) {
    char logMessage[BUFFERSIZE * 3];
    int offset = 0;

    // Iterate over each byte of the data and append its hex representation to logMessage
    for (int i = 0; i < data_length; i++) {
        offset += snprintf(logMessage + offset, sizeof(logMessage) - offset, "%02x ", data[i]);
        if (offset >= sizeof(logMessage)) {
            break;  // Prevent buffer overflow, stop if we exceed the buffer size
        }
    }

    // Ensure the string is null-terminated
    logMessage[offset - 1] = '\0';  // Replace the last space with a null terminator

    prettyLogger(LOG_LEVEL_DEBUG, clientIp, logMessage);
}

void * connectionCb(void * payload) {
  struct ConnectionPayload * connInfo = (struct ConnectionPayload *) payload;
  uint8_t data[BUFFERSIZE];
  char logMessage[2048];
  CommandResponse * responsePayload;
  
  int c = read(connInfo->sockfd, &data, BUFFERSIZE);
  if (c < 0) {
    fprintf(stderr, "ERROR reading from socket: %s\n", strerror(errno));
    close(connInfo->sockfd);
    free(connInfo);
    pthread_exit(NULL);
    return -1;
  }
  
  log_incoming_data(&connInfo->clientIp, (uint8_t *)data, c);

  responsePayload = malloc(sizeof(CommandResponse));
  if (responsePayload == NULL) {
    fprintf(stderr, "Unable to allocate memory for CommandResponse: %s\n", strerror(errno));
    close(connInfo->sockfd);
    free(connInfo);
    pthread_exit(NULL);
    return -1;
  }

  // @todo handle return value, -1 - error
  connInfo->on_message(responsePayload, data, &connInfo->clientIp);

  if (responsePayload->responseLength > 0) {
    int n = 0;
    uint8_t * ptr = (uint8_t*) responsePayload;
    int written = 0;
    int toWrite = responsePayload->responseLength;

    while(responsePayload->responseLength > written)
    {
      n = send(connInfo->sockfd, ptr, (toWrite - written), 0x4000);
      if (n < 0) {
        free(responsePayload);
        fprintf(stderr, "Socket send failed: %s\n", strerror(errno));
        close(connInfo->sockfd);
        free(connInfo);
        pthread_exit(NULL);
        return -1;
      }
      ptr += 1;
      written += n;
    }

    prettyLogger(LOG_LEVEL_DEBUG, "TCP", "Data sent.");     
  }

  free(responsePayload);
  close(connInfo->sockfd);

  free(connInfo);

  pthread_mutex_lock(&GlobaSocketLock);
  connectionWorkers[connInfo->workerId] = 0;
  pthread_mutex_unlock(&GlobaSocketLock);

  // @todo going to be LOG_LEVEL_DEBUG
  snprintf(logMessage, sizeof(logMessage), "%s closed", connInfo->clientIp);
  prettyLogger(LOG_LEVEL_DEBUG, "TCP", logMessage);   

  pthread_exit(NULL);
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

  snprintf(logMessage, sizeof(logMessage), "Listen *:%d", socketConfig->port);
  prettyLogger(LOG_LEVEL_INFO, "TCP", logMessage);

  while(!socketExitRequested)
  {
    int availableSlot = -1;
    for (int j = 0; j < MAX_CONN; j++) {
        if (connectionWorkers[j] == 0) {
            availableSlot = j;
            break;
        }
    }

    if (availableSlot >= 0) {
        struct ConnectionPayload *payload = malloc(sizeof(struct ConnectionPayload));
        if (payload == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            pthread_exit(-1);
        }

        // Accept the data packet from client and verification
        len = sizeof(cli);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli, &len);

        if (newsockfd < 0) { 
          fprintf(stderr, "Server accept failed: %s\n", strerror(errno));
          free(payload);
          pthread_exit(NULL);
          exit(-1);
        }

        inet_ntop( AF_INET, &cli.sin_addr, clientIp, INET_ADDRSTRLEN );   

        strncpy(payload->clientIp, clientIp, sizeof(clientIp));
        payload->sockfd = newsockfd;
        payload->debug = socketConfig->debug;
        payload->on_message = socketConfig->on_message;
        payload->workerId = availableSlot;

        pthread_mutex_lock(&GlobaSocketLock);
        if (pthread_create(&connectionWorkers[availableSlot], NULL, connectionCb, (void *) payload) != 0) {
          free(payload);
          connectionWorkers[availableSlot] = 0;
          fprintf(stderr, "Connection workers init failed: %s\n", strerror(errno));
          pthread_exit(-1);
        }
        pthread_mutex_unlock(&GlobaSocketLock);

    } else {
      prettyLogger(LOG_LEVEL_DEBUG, "TCP", "No more connections left...");
      sleep(1);
    }
  }

  prettyLogger(LOG_LEVEL_INFO, "TCP", "Closing client connections...");
  for (int j = 0; j < MAX_CONN; j++) {
    if (connectionWorkers[j]) {
      pthread_cancel(connectionWorkers[j]);
      pthread_join(connectionWorkers[j], NULL);

      pthread_mutex_lock(&GlobaSocketLock);
      connectionWorkers[j] = 0;
      pthread_mutex_unlock(&GlobaSocketLock);
    }
  }
  prettyLogger(LOG_LEVEL_INFO, "TCP", "Client connections closed.");

  close(sockfd);

  pthread_exit(0);

  return NULL;
}

void startSocketService(struct SocketConfig * config) {
  pthread_mutex_init(&GlobaSocketLock, NULL);

  if (pthread_create(&ServiceThreadId, NULL, startSocketListener, config) != 0)
    ServiceThreadId = 0;
}

void stopSocketService() {
  socketExitRequested = 1;

  if (ServiceThreadId) {
    pthread_join(ServiceThreadId, NULL);
    ServiceThreadId = 0;
  }

  pthread_mutex_destroy(&GlobaSocketLock);
}