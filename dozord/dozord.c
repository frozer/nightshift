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
#include <dozor.h>
#include "answer.h"
#include "command.h"
#include "nightshift-mqtt.h"

#define SA struct sockaddr
#define DEFAULT_PORT 1111
#define DEFAULT_COMMANDS "commands.txt"
#define DEBUG 0
#define VERSION "0.8.1"
#define AGENT_ID "80d7be61-d81d-4aac-9012-6729b6392a89"

struct GlobalArgs_t {
  unsigned int siteId;
  unsigned int port;
  char * pinCode;
  char * commandsFilename;
  unsigned int debug;
} GlobalArgs;
static const char * optString = "p:k:c:s:h?:d";

pthread_mutex_t writelock;

unsigned char * userKey;

static Commands * commands;

struct mosquitto * mosq;
int mosqConnectorId = 0;
bool GlobalMQTTConnected = false;
pthread_t GlobalReconnectThread = 0;

void eventCallback(connectionInfo * conn, EventInfo* eventInfo)
{
  time_t ticks;
  char receivedTimestamp[25] = {0};
  char eventReport[2048] = {0};
  char report[3072] = {0};
  char topic[256] = {0};
  bool retainFlag = false;

  ticks = time(NULL);
  
  sprintf(receivedTimestamp, "%.24s", ctime(&ticks));  
  sprintf(eventReport, PAYLOAD_JSON, conn->clientIp, receivedTimestamp, eventInfo->event);
  sprintf(report, MESSAGE_JSON, AGENT_ID, eventReport);

  switch(eventInfo->eventType)
  {
    case ENUM_EVENT_TYPE_REPORT:
      sprintf(topic, REPORT_TOPIC, eventInfo->siteId);
      break;

    case ENUM_EVENT_TYPE_COMMAND_RESPONSE:
      sprintf(topic, COMMAND_RESULT_TOPIC, eventInfo->siteId);
      break;
    
    case ENUM_EVENT_TYPE_KEEPALIVE:
      sprintf(topic, HEARBEAT_TOPIC, eventInfo->siteId);
      retainFlag = true;
      break;

    case ENUM_EVENT_TYPE_ZONEINFO:
      sprintf(topic, ZONE_TOPIC, eventInfo->siteId, eventInfo->sourceId);
      retainFlag = true;
      break;

    case ENUM_EVENT_TYPE_SECTIONINFO:
      sprintf(topic, SECTION_TOPIC, eventInfo->siteId, eventInfo->sourceId);
      retainFlag = true;
      break;

    case ENUM_EVENT_TYPE_ARM_DISARM:
      sprintf(topic, ARM_DISARM_TOPIC, eventInfo->siteId);
      retainFlag = true;
      break;

    default:    
      sprintf(topic, EVENT_TOPIC, eventInfo->siteId);
  }

  if (GlobalMQTTConnected)
  {
    mosquitto_publish(mosq, NULL, topic, strlen(report), report, 0, retainFlag);
    printf(PAYLOAD_JSON, conn->clientIp, receivedTimestamp, eventInfo->event);
  }  
}

void displayHelp()
{
  printf("dozord %s\nUsage: ./dozord -s <site> -p <port|1111> -k <device pincode> -d\n", VERSION);
}

void getAgentInfo(char* agentInfo)
{
  if (agentInfo == NULL)
  {
    return;
  }

  sprintf(agentInfo, ACK_JSON, VERSION, AGENT_ID, GlobalArgs.siteId);
}

void initCommandsStore()
{
  commands = malloc(sizeof(Commands));
  if (commands == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for commands: %s\n", strerror(errno));
    exit(-1);
  }
  
  commands->length = 0;
}

// handle network connections
void* handle(void* arg) 
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

void* mqtt_thread_reconnect(void* args)
{
  struct mosquitto *mosq = NULL;
	int sleep_time = 30;
	if(args == NULL)
	{
		pthread_exit(0);
		return 0;
	}
	mosq = (struct mosquitto*)args;
	sleep_time += (10 - (rand() % 20));
	sleep(sleep_time);
	if(!GlobalMQTTConnected)
		mosquitto_reconnect_async(mosq);
	pthread_exit(0);
	return 0;
}

void* mqtt_thread_connect(void* args)
{
  struct mosquitto *mosq = NULL;
  char agentInfo[256] = {0};
  char commandTopic[256] = {0};
  sprintf(commandTopic, COMMAND_TOPIC, GlobalArgs.siteId);

	if(args == NULL)
	{
		pthread_exit(0);
		return NULL;
	}

  getAgentInfo(agentInfo);

	mosq = (struct mosquitto *) args;
  
  mosquitto_subscribe(mosq, NULL, commandTopic, 0);

  mosquitto_publish(mosq, NULL, ACK_TOPIC, strlen(agentInfo), agentInfo, 0, false);

  pthread_exit(0);
  return NULL;
}

void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
  if (GlobalReconnectThread)
  {
    pthread_cancel(GlobalReconnectThread);
    pthread_join(GlobalReconnectThread, NULL);
    GlobalReconnectThread = 0;
  }

  if (result == 0)
  {
    pthread_t conn = 0;
    if (GlobalArgs.debug)
    {
      printf("*** connected to %s:%d, rc=%d\n", MQTT_HOST, MQTT_PORT, result);
    }

    GlobalMQTTConnected = true;
    if (pthread_create(&conn, NULL, mqtt_thread_connect, mosq) == 0)
      pthread_detach(conn);
  } else {
    printf("*** broker connection lost\n");
    GlobalMQTTConnected = false;
    if (pthread_create(&GlobalReconnectThread, NULL, mqtt_thread_reconnect, mosq) != 0)
      GlobalReconnectThread = 0;
  }
}

void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	pthread_mutex_lock(&writelock);
  readCommandsFromString(commands, (char *)message->payload, GlobalArgs.debug);
  pthread_mutex_unlock(&writelock);
}

void* dozor_thread_listener()
{
  int sockfd, newsockfd, len, pid, rc; 
  int port;
  char clientIp[INET_ADDRSTRLEN];
	struct sockaddr_in servaddr, cli; 
  connectionInfo infos[5];
  pthread_t connectionWorkers[5];
  pthread_t watcherWorker;
  unsigned short int i = 0;

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

  while(true)
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
}

pthread_t startDozorListener()
{
  pthread_t thread = 0;
  if (pthread_create(&thread, NULL, dozor_thread_listener, NULL) != 0)
    thread = 0;
  return thread;
}

int initializeMQTT()
{
  char clientId[24] = {0};
  char willTopic[28] = {0};
  char willMessage[36] = {0};
  bool retainFlag = true;
  int rc = 0;

  mosquitto_lib_init();
  
  sprintf(clientId, "nightshift_%d", getpid());
  sprintf(willTopic, DISCONNECTED_TOPIC, GlobalArgs.siteId);
  sprintf(willMessage, WILL_MESSAGE, GlobalArgs.siteId);

  mosq = mosquitto_new(clientId, 1, 0);
  if (mosq)
  {    
    mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
    mosquitto_message_callback_set(mosq, mqtt_message_callback);

    mosquitto_will_set(mosq, willTopic, sizeof(willMessage), willMessage, 0, retainFlag);

    mosquitto_loop_start(mosq);
    rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS)
    {
      printf("*** Unable to connect to broker at %s:%d!\n", MQTT_HOST, MQTT_PORT);
    } else {
      if (GlobalArgs.debug)
      {
        printf("*** Connected to broker at %s:%d!\n", MQTT_HOST, MQTT_PORT);
      }
    }

    return rc;
  }
  return rc;
}

// Driver function 
int main(int argc, char **argv) 
{ 
  int opt;
  char * commandFilename;
  pthread_t watcherWorker;

  GlobalArgs.port = DEFAULT_PORT;
  GlobalArgs.siteId = 0;
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

      case 's':
        GlobalArgs.siteId = strtol(optarg, 0, 10);
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

  if (GlobalArgs.siteId == 0)
  {
    printf("Guard device ID is not set. Exiting.\n");
		return 0;  
  }

  if (GlobalArgs.pinCode == NULL)
  {
    printf("Guard device pincode is not set. Exiting.\n");
		return 0;  
  }

  pthread_mutex_init(&writelock, NULL);

  if (GlobalArgs.debug) {
    printf("Listen %d\n", GlobalArgs.port);
  }

  // start Dozor listener server on specified port
  startDozorListener();

  initCommandsStore();

  // initialize MQTT
  initializeMQTT();

  pthread_mutex_destroy(&writelock);

  pthread_exit(NULL);

  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
} 
