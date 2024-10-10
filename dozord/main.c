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
#include "command.h"
#include "nightshift-mqtt.h"
#include "logger.h"
#include "app-config.h"
#include "socket-server.h"

#define SA struct sockaddr
#define DEFAULT_ANSWER ""

static const char * optString = "l:k:s:m:p:h?:d";

pthread_mutex_t commandsWriteLock;

static struct AppConfig appConfig;
static Commands * commands;

volatile sig_atomic_t exitRequested = 0;

void term(int signum)
{
   exitRequested = 1;
}

struct Event {
  char * deviceIp;
  EventInfo * data;
};

void * publishEvent(void * args)
{
  struct Event * payload = (struct Event *) args;
  time_t ticks;
  char receivedTimestamp[25] = {0};
  char eventReport[2048] = {0};
  char report[3072] = {0};
  char topic[256] = {0};
  bool retainFlag = false;

  ticks = time(NULL);
  
  sprintf(receivedTimestamp, "%.24s", ctime(&ticks));  
  sprintf(eventReport, PAYLOAD_JSON, payload->deviceIp, receivedTimestamp, payload->data->event);
  sprintf(report, MESSAGE_JSON, AGENT_ID, eventReport);

  switch(payload->data->eventType)
  {
    case ENUM_EVENT_TYPE_REPORT:
      sprintf(topic, REPORT_TOPIC, payload->data->siteId);
      break;

    case ENUM_EVENT_TYPE_COMMAND_RESPONSE:
      sprintf(topic, COMMAND_RESULT_TOPIC, payload->data->siteId);
      break;
    
    case ENUM_EVENT_TYPE_KEEPALIVE:
      sprintf(topic, HEARBEAT_TOPIC, payload->data->siteId);
      retainFlag = true;
      break;

    case ENUM_EVENT_TYPE_ZONEINFO:
      sprintf(topic, ZONE_TOPIC, payload->data->siteId, payload->data->sourceId);
      retainFlag = true;
      break;

    case ENUM_EVENT_TYPE_SECTIONINFO:
      sprintf(topic, SECTION_TOPIC, payload->data->siteId, payload->data->sourceId);
      retainFlag = true;
      break;

    case ENUM_EVENT_TYPE_ARM_DISARM:
      sprintf(topic, ARM_DISARM_TOPIC, payload->data->siteId);
      retainFlag = true;
      break;

    default:    
      sprintf(topic, EVENT_TOPIC, payload->data->siteId);
  }

  publish(topic, report, retainFlag);

  free(payload->deviceIp);
  free(payload->data);
  free(payload);

  pthread_exit(0);
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

int socket_message_callback(CommandResponse * response, uint8_t * data, char * clientIp) {
  unsigned short int index = 0;
  short int res = -1;
  char logMessage[2048];
  
  struct Event *payload = malloc(sizeof(struct Event));
  if (payload == NULL) {
    fprintf(stderr, "Unable to allocate memory for Event: %s\n", strerror(errno));
    return -1;
  }

  payload->deviceIp = malloc(16 * sizeof(char));
  if (payload->deviceIp == NULL) {
    fprintf(stderr, "Unable to allocate memory for Event device ip: %s\n", strerror(errno));
    free(payload);
    return -1;
  }

  payload->data = malloc(sizeof(EventInfo));
  if (payload->data == NULL) {
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    free(payload->deviceIp);
    free(payload);
    return -1;
  }

  CryptoSession * crypto = malloc(sizeof(CryptoSession));
  if (crypto == NULL)
  {
    fprintf(stderr, "Unable to allocate memory for crypto session: %s\n", strerror(errno));
    free(payload->deviceIp);
    free(payload->data);
    free(payload);
    return -1;
  }

  Events * events = dozor_unpackV2(crypto, data, appConfig.pinCode, appConfig.debug);

  if (events != NULL && events->errorCode == 0) {
    for (index = 0; index < events->length; index++) {
      pthread_t publishThread = 0;

      snprintf(logMessage, sizeof(logMessage), "%s", events->items[index].event);
      logger(LOG_LEVEL_INFO, clientIp, logMessage);
      
      strncpy(payload->deviceIp, clientIp, sizeof(char) * 16);
      memcpy(payload->data, &events->items[0], sizeof(EventInfo));

      if (pthread_create(&publishThread, NULL, publishEvent, payload) == 0)
        pthread_detach(publishThread);
    }

    free(events);

    pthread_mutex_lock(&commandsWriteLock);
    short int found = getNextCommandIdx(commands);
    if (found != -1) {
      if ((commands->items[found].done != 1) && (strlen(commands->items[found].value) > 1)) {
        res = dozor_pack(response, crypto, commands->items[found].id, commands->items[found].value, appConfig.debug);
      } else {
        res = dozor_pack(response, crypto, 1, DEFAULT_ANSWER, appConfig.debug);
      }
    } else {
      res = dozor_pack(response, crypto, 1, DEFAULT_ANSWER, appConfig.debug);
    }
    pthread_mutex_unlock(&commandsWriteLock);

    if (response == NULL || res == -1) {
      free(payload->deviceIp);
      free(payload->data);
      free(payload);
      free(crypto);

      fprintf(stderr, "Unable to encrypt command!\n");
      return -1;
    }
  } else {
    // @todo handle error code value
    free(payload->deviceIp);
    free(payload->data);
    free(payload);
  }
  free(crypto);
}

void * mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
  char logMessage[256];
  snprintf(logMessage, sizeof(logMessage), "MQTT:: New message received, topic - \"%s\", \"%s\"", (char *) message->topic, (char *) message->payload);
  logger(LOG_LEVEL_INFO, "-", logMessage);
	
  pthread_mutex_lock(&commandsWriteLock);
  
  readCommandsFromString(commands, (char *)message->payload);

  pthread_mutex_unlock(&commandsWriteLock);
}

int main(int argc, char **argv) 
{
  // handle SIG
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  
  initializeAppConfig(&appConfig);
  appConfig.socketConfig.on_message = socket_message_callback;

  processCommandLineOptions(argc, argv, &appConfig);

  if (appConfig.mqttConfig.siteId == 0)
  {
    logger(LOG_LEVEL_ERROR, "-", "Guard device ID is not set. Exiting.");
		return 0;  
  }

  if (appConfig.pinCode == "")
  {
    logger(LOG_LEVEL_ERROR, "-", "Guard device pincode is not set. Exiting.");
		return 0;  
  }

  char logMessage[256];
  snprintf(logMessage, sizeof(logMessage), "Guard device ID %d", appConfig.mqttConfig.siteId);
  logger(LOG_LEVEL_INFO, "-", logMessage);

  pthread_mutex_init(&commandsWriteLock, NULL);

  initCommandsStore();

  initializeMQTT(&appConfig.mqttConfig, mqtt_message_callback);

  startSocketService(&appConfig.socketConfig);

  while (!exitRequested) {
    
  }
  
  pthread_mutex_destroy(&commandsWriteLock);

  stopSocketService();
  
  disconnectMQTT();

  pthread_exit(NULL);

  free(commands);
  
  return 0;
} 
