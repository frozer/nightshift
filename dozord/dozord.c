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
#include <sys/inotify.h>
#include <dozor.h>
#include "answer.h"
#include "command.h"
#include "event-stream.h"

#define SA struct sockaddr
#define DEFAULT_PORT 1111
#define DEFAULT_COMMANDS "commands.txt"
#define DEBUG 0
#define VERSION "0.8.1"

struct GlobalArgs_t {
  unsigned int port;
  char * pinCode;
  char * commandsFilename;
  unsigned int debug;
} GlobalArgs;
static const char * optString = "p:k:c:h?:d";

pthread_mutex_t writelock;

unsigned char * userKey;

static Commands * commands;

void displayHelp()
{
  printf("dozord %s\nUsage: ./dozord -p <port|1111> -k <device pincode> -c <commands file|commands.txt> -d\n", VERSION);
}

// handle commands file changes
void * commandWatcherHandler(void * arg)
{
  char * fn = (char *)arg;
  int fd, wd;

  pthread_mutex_lock(&writelock);
  
  commands = malloc(sizeof(Commands));
  if (commands == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for commands: %s\n", strerror(errno));
    return (void *) -1;
  }
  
  commands->length = 0;

  readCommandsFromFile(commands, fn, GlobalArgs.debug);

  pthread_mutex_unlock(&writelock); 
  

  fd = inotify_init();
  if (fd < 0)
  {
    perror("Error inotify initialization!");
    free(commands);
    return (void *) -1;
  }
  
  wd = inotify_add_watch( fd, fn, IN_MODIFY);
  
  if (wd<0)
  {
    perror("Error add watch");
    free(commands);
    return (void *) -1;
  }

  while(1) {
    char buffer[4096];
    struct inotify_event *event = NULL;
    int exec = 0;

    int len = read(fd, buffer, sizeof(buffer));
    if (len < 0) {
        pthread_mutex_lock(&writelock);
        free(commands);
        pthread_mutex_unlock(&writelock);
        fprintf(stderr, "read: %s\n", strerror(errno));
        exit(-1);
    }

    event = (struct inotify_event *) buffer;
    while(event != NULL) {
        if ( (event->mask & IN_MODIFY) && event->len > 0) {
            // printf("File Modified: %s\n", event->name);
        } else {
            pthread_mutex_lock(&writelock);
            readCommandsFromFile(commands, fn, GlobalArgs.debug);
            pthread_mutex_unlock(&writelock);
        }

        /* Move to next struct */
        len -= sizeof(*event) + event->len;
        if (len > 0)
            event = ((void *) event) + sizeof(event) + event->len;
        else
            event = NULL;
    }
  }
  inotify_rm_watch( fd, wd );

  pthread_mutex_lock(&writelock);
  free(commands);
  pthread_mutex_unlock(&writelock);
  return 0;
}

// handle network connections
void * handle(void * arg) 
{
  int c;
  connectionInfo * conn = (connectionInfo*) arg;
  uint8_t packet[BUFFERSIZE];
  int sockfd = conn->sock;

  CryptoSession * crypto = malloc(sizeof(CryptoSession));
  if (crypto == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    close(sockfd);
    return 0; 
  }

  c = read(sockfd, &packet, BUFFERSIZE);
  if (c < 0) {
    fprintf(stderr, "ERROR reading from socket: %s\n", strerror(errno));
    free(crypto);
    close(sockfd);
    return 0;
  }

  int res = dozor_unpack(crypto, conn, packet, &eventCallback, conn->debug);
  if (res < 0) {
    free(crypto);
    close(sockfd);
    return 0;
  }
  
  pthread_mutex_lock(&writelock);

  short int commandId = answerDevice(sockfd, crypto, commands, conn->debug);

  if (commands != NULL) {
    if (commandId != -1 && commandId < commands->length)
    {
      commands->items[commandId].done = 1;  
    }
  }
  pthread_mutex_unlock(&writelock);

  close(sockfd);

  free(crypto);
  return (void *) 0;
} 

// Driver function 
int main(int argc, char **argv) 
{ 
  int opt;
	int sockfd, newsockfd, len, pid, rc; 
  int port;
  char clientIp[INET_ADDRSTRLEN];
  char * commandFilename;
	struct sockaddr_in servaddr, cli; 
  connectionInfo infos[5];
  pthread_t connectionWorkers[5];
  pthread_t watcherWorker;
  unsigned short int i = 0;

  GlobalArgs.port = DEFAULT_PORT;
  GlobalArgs.pinCode = NULL;
  GlobalArgs.commandsFilename = DEFAULT_COMMANDS;
  GlobalArgs.debug = DEBUG;

  if (argc < 2)
  {
    printf("Usage: ./dozord -h\n");
    return 0;
  }

  while((opt = getopt(argc, argv, optString)) != -1)  
  {  
    switch(opt)  
    {  
      case 'p':
        GlobalArgs.port = strtol(optarg, 0, 10);
        break;

      case 'k':
        GlobalArgs.pinCode = optarg;
        break;

      case 'c':
        GlobalArgs.commandsFilename = optarg;
        break;

      case 'h':
      case '?':
        displayHelp();
        exit(0);
        break;

      case 'd':
        GlobalArgs.debug = 1;
        break;

      default:
        abort();
    }
  }

  if (GlobalArgs.pinCode == NULL)
  {
    printf("Guard device pincode is not set. Exiting.\n");
		return 0;  
  }

  pthread_mutex_init(&writelock, NULL);

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		fprintf(stderr, "Socket create failed: %s\n", strerror(errno));
		exit(-1); 
	}
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(GlobalArgs.port); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		fprintf(stderr, "Socket bind failed: %s\n", strerror(errno));
		exit(-1); 
	}

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		fprintf(stderr, "Socket listen failed: %s\n", strerror(errno));
		exit(-1); 
	}

  if (GlobalArgs.debug) {
    printf("Listen %d\n", GlobalArgs.port);
  }

	len = sizeof(cli); 

  // initiate watcher
  rc = pthread_create(&watcherWorker, NULL, commandWatcherHandler, (void *) GlobalArgs.commandsFilename);
  if (rc)
  {
    fprintf(stderr, "Watcher worker failed: %s\n", strerror(errno));
    exit(-1);
  }

  if (GlobalArgs.debug) {
    printf("Commands file %s\n", GlobalArgs.commandsFilename);
  }

  while(1)
  {
    // Accept the data packet from client and verification 
    newsockfd = accept(sockfd, (SA*)&cli, &len);

    if (newsockfd < 0) { 
      fprintf(stderr, "Server accept failed: %s\n", strerror(errno));
      pthread_exit(NULL);
      exit(-1);
    }

    inet_ntop( AF_INET, &cli.sin_addr, clientIp, INET_ADDRSTRLEN );   
    
    if (GlobalArgs.debug) {
      printf("New connection from: %s\n", clientIp);
    }

    strcpy(infos[i].clientIp, clientIp);
    strcpy(infos[i].pinCode, GlobalArgs.pinCode);
    infos[i].sock = newsockfd;
    infos[i].debug = GlobalArgs.debug;    

    rc = pthread_create(&connectionWorkers[i], NULL, handle, (void *) &infos[i]);
    
    if (rc)
    {
      fprintf(stderr, "Connection workers init failed: %s\n", strerror(errno));
      exit(-1);
    }
    
    i++;
    if (i >= 4)
    {
      i = 0;
      while (i < 4)
      {
        pthread_join(connectionWorkers[i++], NULL);
      }
      i = 0;
    }
  }

  pthread_mutex_destroy(&writelock);
  pthread_exit(NULL);
} 
