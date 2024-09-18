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
#include <stdbool.h>
#include <mosquitto.h>
#include <signal.h>
#include <dozor.h>
#include "answer.h"
#include "command.h"
#include "nightshift-mqtt.h"
#include "logger.h"

#define SA struct sockaddr
#define DEFAULT_PORT 1111
#define DEFAULT_COMMANDS "commands.txt"
#define DEBUG 0
#define VERSION "0.8.1"
#define AGENT_ID "80d7be61-d81d-4aac-9012-6729b6392a89"

struct AppConfig {
  unsigned int siteId;
  unsigned int port;
  char pinCode[10];
  struct MQTTConfig mqttConfig;
  unsigned int debug;
};

static const char * optString = "l:k:s:m:p:h?:d";

pthread_mutex_t writelock;

// unsigned char * userKey;

// static Commands * commands;

volatile sig_atomic_t exitRequested = 0;

void term(int signum)
{
   exitRequested = 1;
   
  //  mosquitto_loop_stop(mosq, &(int){1});
}

// void eventCallback(connectionInfo * conn, EventInfo* eventInfo)
// {
//   time_t ticks;
//   char receivedTimestamp[25] = {0};
//   char eventReport[2048] = {0};
//   char report[3072] = {0};
//   char topic[256] = {0};
//   bool retainFlag = false;

//   ticks = time(NULL);
  
//   sprintf(receivedTimestamp, "%.24s", ctime(&ticks));  
//   sprintf(eventReport, PAYLOAD_JSON, conn->clientIp, receivedTimestamp, eventInfo->event);
//   sprintf(report, MESSAGE_JSON, AGENT_ID, eventReport);

//   switch(eventInfo->eventType)
//   {
//     case ENUM_EVENT_TYPE_REPORT:
//       sprintf(topic, REPORT_TOPIC, eventInfo->siteId);
//       break;

//     case ENUM_EVENT_TYPE_COMMAND_RESPONSE:
//       sprintf(topic, COMMAND_RESULT_TOPIC, eventInfo->siteId);
//       break;
    
//     case ENUM_EVENT_TYPE_KEEPALIVE:
//       sprintf(topic, HEARBEAT_TOPIC, eventInfo->siteId);
//       retainFlag = true;
//       break;

//     case ENUM_EVENT_TYPE_ZONEINFO:
//       sprintf(topic, ZONE_TOPIC, eventInfo->siteId, eventInfo->sourceId);
//       retainFlag = true;
//       break;

//     case ENUM_EVENT_TYPE_SECTIONINFO:
//       sprintf(topic, SECTION_TOPIC, eventInfo->siteId, eventInfo->sourceId);
//       retainFlag = true;
//       break;

//     case ENUM_EVENT_TYPE_ARM_DISARM:
//       sprintf(topic, ARM_DISARM_TOPIC, eventInfo->siteId);
//       retainFlag = true;
//       break;

//     default:    
//       sprintf(topic, EVENT_TOPIC, eventInfo->siteId);
//   }

//   if (GlobalMQTTConnected)
//   {
//     mosquitto_publish(mosq, NULL, topic, strlen(report), report, 0, retainFlag);
//     printf(PAYLOAD_JSON, conn->clientIp, receivedTimestamp, eventInfo->event);
//   }  
// }

void displayHelp()
{
  printf("dozord %s\nUsage: ./dozord -s <site> -l <port|1111> -k <device pincode> -m <MQTT host|127.0.0.1> -p <MQTT port|1883> -d\n", VERSION);
}

// void initCommandsStore()
// {
//   commands = malloc(sizeof(Commands));
//   if (commands == NULL)
//   {
//     fprintf(stderr, "Unable to allocate memory for commands: %s\n", strerror(errno));
//     exit(-1);
//   }
  
//   commands->length = 0;
// }

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

void * mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
  char logMessage[256];
  snprintf(logMessage, sizeof(logMessage), "MQTT:: New message received, topic - \"%s\", \"%s\"", (char *) message->topic, (char *) message->payload);
  logger(LOG_LEVEL_INFO, logMessage);
	
  pthread_mutex_lock(&writelock);
  
  // @todo
  // readCommandsFromString(commands, (char *)message->payload, GlobalArgs.debug);

  pthread_mutex_unlock(&writelock);
}

// void* dozor_thread_listener()
// {
//   int sockfd, newsockfd, pid, rc; 
//   int port;
//   char clientIp[INET_ADDRSTRLEN];
// 	struct sockaddr_in servaddr = {0};
  
//   struct sockaddr_in cli = {0};
//   socklen_t len;

//   connectionInfo infos[5];
//   pthread_t connectionWorkers[5];
//   pthread_t watcherWorker;
//   unsigned short int i = 0;

//   // socket create and verification 
// 	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
// 	if (sockfd == -1) { 
// 		fprintf(stderr, "Socket create failed: %s\n", strerror(errno));
// 		exit(-1); 
// 	}

//   // set sock options
//   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1)
//   {
//     fprintf(stderr, "Socket options setup failed: %s\n", strerror(errno));
//     exit(-1);
//   }

// 	bzero(&servaddr, sizeof(servaddr)); 

// 	// assign IP, PORT
// 	servaddr.sin_family = AF_INET; 
// 	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
// 	servaddr.sin_port = htons(GlobalArgs.port); 

// 	// Binding newly created socket to given IP and verification 
// 	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
// 		fprintf(stderr, "Socket bind failed: %s\n", strerror(errno));
// 		exit(-1); 
// 	}

// 	// Now server is ready to listen and verification 
// 	if ((listen(sockfd, 5)) != 0) { 
// 		fprintf(stderr, "Socket listen failed: %s\n", strerror(errno));
// 		exit(-1); 
// 	}

//   while(!exitRequested)
//   {
//     len = sizeof(cli);

//     // Accept the data packet from client and verification 
//     newsockfd = accept(sockfd, (SA*)&cli, &len);

//     if (newsockfd < 0) { 
//       fprintf(stderr, "Server accept failed: %s\n", strerror(errno));
//       pthread_exit(NULL);
//       exit(-1);
//     }

//     inet_ntop( AF_INET, &cli.sin_addr, clientIp, INET_ADDRSTRLEN );   
        
//     if (GlobalArgs.debug) {
//       printf("New connection from: %s\n", clientIp);
//     }

//     strcpy(infos[i].clientIp, clientIp);
//     strcpy(infos[i].pinCode, GlobalArgs.pinCode);
//     infos[i].sock = newsockfd;
//     infos[i].debug = GlobalArgs.debug;    

//     rc = pthread_create(&connectionWorkers[i], NULL, handle, (void *) &infos[i]);
    
//     if (rc)
//     {
//       fprintf(stderr, "Connection workers init failed: %s\n", strerror(errno));
//       exit(-1);
//     }
    
//     i++;
//     if (i >= 4)
//     {
//       i = 0;
//       while (i < 4)
//       {
//         pthread_join(connectionWorkers[i++], NULL);
//       }
//       i = 0;
//     }
//   }
// }

// pthread_t startDozorListener()
// {
//   pthread_t thread = 0;
//   if (pthread_create(&thread, NULL, dozor_thread_listener, NULL) != 0)
//     thread = 0;
//   return thread;
// }


int main(int argc, char **argv) 
{ 
  struct mosquitto * mosq;
  struct MQTTConfig mqttConfig;

  int opt;
  // pthread_t watcherWorker;

  // handle SIG
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  struct AppConfig appConfig;
  appConfig.port = DEFAULT_PORT;
  appConfig.siteId = 0;
  appConfig.mqttConfig.siteId = 0;
  strncpy(appConfig.pinCode, "", sizeof(appConfig.pinCode));
  strncpy(appConfig.mqttConfig.host, MQTT_HOST, sizeof(appConfig.mqttConfig.host));
  appConfig.mqttConfig.port = MQTT_PORT;
  appConfig.mqttConfig.debug = DEBUG;
  strncpy(appConfig.mqttConfig.agentId, AGENT_ID, sizeof(appConfig.mqttConfig.agentId));
  appConfig.debug = DEBUG;

  if (argc < 2)
  {
    printf("Usage: ./dozord -h\n");
    return 0;
  }

  while((opt = getopt(argc, argv, optString)) != -1)  
  {  
    switch(opt)  
    {  
      case 'l':
        appConfig.port = strtol(optarg, 0, 10);
        break;

      case 'k':
        strncpy(appConfig.pinCode, optarg, sizeof(appConfig.pinCode));
        break;

      case 's':
        appConfig.siteId = strtol(optarg, 0, 10);
        appConfig.mqttConfig.siteId = appConfig.siteId;
        break;

      case 'm':
        strncpy(appConfig.mqttConfig.host, optarg, sizeof(appConfig.mqttConfig.host));
        break;

      case 'p':
        appConfig.mqttConfig.port = strtol(optarg, 0, 10);
        break;

      case 'h':
      case '?':
        displayHelp();
        exit(0);
        break;

      case 'd':
        appConfig.mqttConfig.debug = 1;
        appConfig.debug = 1;
        break;

      default:
        abort();
    }
  }

  if (appConfig.siteId == 0)
  {
    logger(LOG_LEVEL_ERROR, "Guard device ID is not set. Exiting.");
		return 0;  
  }

  if (appConfig.pinCode == "")
  {
    logger(LOG_LEVEL_ERROR, "Guard device pincode is not set. Exiting.");
		return 0;  
  }

  char logMessage[256];
  snprintf(logMessage, sizeof(logMessage), "Listen %d, guard device ID %d", appConfig.port, appConfig.siteId);
  logger(LOG_LEVEL_INFO, logMessage);

  pthread_mutex_init(&writelock, NULL);

  // start Dozor listener server on specified port
  // startDozorListener();

  // initCommandsStore();

  // initialize MQTT
  initializeMQTT(&appConfig.mqttConfig, mosq, mqtt_message_callback);

  pthread_mutex_destroy(&writelock);

  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  pthread_exit(NULL);
} 
