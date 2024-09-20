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
#include "socket-server.h"

#define SA struct sockaddr
#define DEFAULT_PORT 1111
#define DEFAULT_COMMANDS "commands.txt"
#define DEBUG 0
#define VERSION "0.8.1"
#define AGENT_ID "80d7be61-d81d-4aac-9012-6729b6392a89"

struct AppConfig {
  struct SocketConfig socketConfig;
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

void * socket_message_callback() {

}

void * mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
  char logMessage[256];
  snprintf(logMessage, sizeof(logMessage), "MQTT:: New message received, topic - \"%s\", \"%s\"", (char *) message->topic, (char *) message->payload);
  logger(LOG_LEVEL_INFO, "-", logMessage);
	
  pthread_mutex_lock(&writelock);
  
  // @todo
  // readCommandsFromString(commands, (char *)message->payload, GlobalArgs.debug);

  pthread_mutex_unlock(&writelock);
}

int main(int argc, char **argv) 
{ 
  int opt;
  // pthread_t watcherWorker;

  // handle SIG
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  struct AppConfig appConfig;

  appConfig.socketConfig.port = DEFAULT_PORT;
  appConfig.socketConfig.siteId = 0;
  strncpy(appConfig.socketConfig.pinCode, "", sizeof(appConfig.socketConfig.pinCode));
  appConfig.socketConfig.debug = DEBUG;
  appConfig.socketConfig.on_message = socket_message_callback;

  appConfig.mqttConfig.siteId = 0;
  strncpy(appConfig.mqttConfig.host, MQTT_HOST, sizeof(appConfig.mqttConfig.host));
  appConfig.mqttConfig.port = MQTT_PORT;
  appConfig.mqttConfig.debug = DEBUG;
  strncpy(appConfig.mqttConfig.agentId, AGENT_ID, sizeof(appConfig.mqttConfig.agentId));

  

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
        appConfig.socketConfig.port = strtol(optarg, 0, 10);
        break;

      case 'k':
        strncpy(appConfig.socketConfig.pinCode, optarg, sizeof(appConfig.socketConfig.pinCode));
        break;

      case 's':
        appConfig.socketConfig.siteId = strtol(optarg, 0, 10);
        appConfig.mqttConfig.siteId = appConfig.socketConfig.siteId;
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
        appConfig.socketConfig.debug = 1;
        break;

      default:
        abort();
    }
  }

  if (appConfig.socketConfig.siteId == 0)
  {
    logger(LOG_LEVEL_ERROR, "-", "Guard device ID is not set. Exiting.");
		return 0;  
  }

  if (appConfig.socketConfig.pinCode == "")
  {
    logger(LOG_LEVEL_ERROR, "-", "Guard device pincode is not set. Exiting.");
		return 0;  
  }

  char logMessage[256];
  snprintf(logMessage, sizeof(logMessage), "Guard device ID %d", appConfig.socketConfig.siteId);
  logger(LOG_LEVEL_INFO, "-", logMessage);

  pthread_mutex_init(&writelock, NULL);

  // initCommandsStore();

  // initialize MQTT
  initializeMQTT(&appConfig.mqttConfig, mqtt_message_callback);

  startSocketService(&appConfig.socketConfig);

  while (!exitRequested) {
    
  }
  
  pthread_mutex_destroy(&writelock);

  stopSocketService();
  
  disconnectMQTT();

  pthread_exit(NULL);

  return 0;
} 
