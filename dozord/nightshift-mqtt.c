#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <mosquitto.h>
#include "nightshift-mqtt.h"
#include "logger.h"

bool GlobalMQTTConnected = false;
bool shouldReconnect = false;
pthread_t GlobalReconnectThread = 0;
pthread_mutex_t GlobalMQTTConnectedLock;
struct mosquitto * mosq = NULL;

void publish(char * topic, char * message, bool retainFlag) {
  int rc = 0;
  char logMessage[2048];

  if (GlobalMQTTConnected) {
    rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 0, retainFlag);
    if (rc != MOSQ_ERR_SUCCESS) {
      snprintf(logMessage, sizeof(logMessage), "Failed to publish to topic \"%s\". Error code: %d", topic, rc);
      prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
    }
  } else {
    // @todo build outgoing queue
  }
}

void* mqtt_thread_reconnect(void* args)
{
  char logMessage[256];
  int rc = 0;

  pthread_mutex_lock(&GlobalMQTTConnectedLock);
  if (!shouldReconnect) {
    pthread_mutex_unlock(&GlobalMQTTConnectedLock);
    pthread_exit(0);
  }
  pthread_mutex_unlock(&GlobalMQTTConnectedLock);

	int sleep_time = MQTT_RECONNECT_SEC;
	
  if(args == NULL) {
		pthread_exit(0);
		return 0;
	}
		
  if (mosq == NULL) {
    pthread_exit(0);
		return 0;
  }

  sleep_time += rand() % 20;

  snprintf(logMessage, sizeof(logMessage), "Connection lost. Reconnecting... %d sec", sleep_time);
  prettyLogger(LOG_LEVEL_INFO, "MQTT", logMessage);

	sleep(sleep_time);

  pthread_mutex_lock(&GlobalMQTTConnectedLock);
  if (!shouldReconnect) {
    pthread_mutex_unlock(&GlobalMQTTConnectedLock);
    pthread_exit(0);
  }

	if (!GlobalMQTTConnected) {
    pthread_mutex_unlock(&GlobalMQTTConnectedLock);
		
    rc = mosquitto_reconnect_async(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
      snprintf(logMessage, sizeof(logMessage), "Failed to reconnect. Error code: %d", rc);
      prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
    }
  }

  pthread_mutex_unlock(&GlobalMQTTConnectedLock);
	pthread_exit(0);
}

void* mqtt_thread_connect(void* args)
{
  struct MQTTConfig * mqttConfig = (struct MQTTConfig *) args;
  char agentInfo[90] = {0};
  char commandTopic[100] = {0};
  char logMessage[256]; 
  int rc = 0;

  if (mqttConfig == NULL) {
		pthread_exit(0);
		return NULL;
  }

  snprintf(commandTopic, sizeof(commandTopic), COMMAND_TOPIC, mqttConfig->siteId);
  snprintf(agentInfo, sizeof(agentInfo), ACK_JSON, mqttConfig->agentId, mqttConfig->siteId);

  rc = mosquitto_subscribe(mosq, NULL, commandTopic, 0);
  if (rc != MOSQ_ERR_SUCCESS) {
    snprintf(logMessage, sizeof(logMessage), "Failed to subscribe to command topic \"%s\". Error code: %d", commandTopic, rc);
    prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
  } else {
    snprintf(logMessage, sizeof(logMessage), "Command topic \"%s\" subscribed", commandTopic);
    prettyLogger(LOG_LEVEL_INFO, "MQTT", logMessage);
  }

  publish(ACK_TOPIC, agentInfo, false);

  pthread_exit(0);
  return NULL;
}

void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
  char logMessage[256];
  struct MQTTConfig * mqttConfig = (struct MQTTConfig *) obj;

  if (GlobalReconnectThread) {
    pthread_join(GlobalReconnectThread, NULL);
    GlobalReconnectThread = 0;
  }

  if (result == 0)
  {
    pthread_t conn = 0;

    snprintf(logMessage, sizeof(logMessage), "Connected %s:%d", mqttConfig->host, mqttConfig->port);
    prettyLogger(LOG_LEVEL_INFO, "MQTT", logMessage);

    pthread_mutex_lock(&GlobalMQTTConnectedLock);
    shouldReconnect = false;
    GlobalMQTTConnected = true;
    pthread_mutex_unlock(&GlobalMQTTConnectedLock);
   
    if (pthread_create(&conn, NULL, mqtt_thread_connect, mqttConfig) == 0)
      pthread_detach(conn);
  
  } else {
    
    prettyLogger(LOG_LEVEL_ERROR, "MQTT", "connection lost. Reconnecting...");

    pthread_mutex_lock(&GlobalMQTTConnectedLock);
    shouldReconnect = true;
    GlobalMQTTConnected = false;
    pthread_mutex_unlock(&GlobalMQTTConnectedLock);

    if (pthread_create(&GlobalReconnectThread, NULL, mqtt_thread_reconnect, NULL) != 0)
      GlobalReconnectThread = 0;
  }
}

void initializeMQTT(struct MQTTConfig* mqttConfig, void (*on_message))
{
  char clientId[24] = {0};
  char willTopic[36] = {0};
  char willMessage[36] = {0};
  bool retainFlag = true;
  char logMessage[256];
  
  int rc = 0;

  pthread_mutex_init(&GlobalMQTTConnectedLock, NULL);

  mosquitto_lib_init();
  
  sprintf(clientId, "nightshift_%d", getpid());
  sprintf(willTopic, DISCONNECTED_TOPIC, mqttConfig->siteId);
  sprintf(willMessage, WILL_MESSAGE, mqttConfig->siteId);

  mosq = mosquitto_new(clientId, 1, mqttConfig);

  if (mosq != NULL)
  {    
    mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
    mosquitto_message_callback_set(mosq, on_message);

    mosquitto_will_set(mosq, willTopic, sizeof(willMessage), willMessage, 0, retainFlag);

    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS)
    {
      snprintf(logMessage, sizeof(logMessage), "Unable to init MQTT %d", rc);
      prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
    }

    rc = mosquitto_connect(mosq, mqttConfig->host, mqttConfig->port, MQTT_KEEPALIVE_SEC);
    
    if (rc != MOSQ_ERR_SUCCESS)
    {
      snprintf(logMessage, sizeof(logMessage), "Unable to connect %s:%d", mqttConfig->host, mqttConfig->port);
      prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);   
    }

  } else {
    prettyLogger(LOG_LEVEL_ERROR, "MQTT", "Failed to create new Mosquitto instance.");
    mosquitto_lib_cleanup();  
  }
}



void disconnectMQTT() {
  int rc = 0;
  char logMessage[256];

  prettyLogger(LOG_LEVEL_INFO, "MQTT", "Closing connection...");

  if (GlobalReconnectThread) {
    pthread_join(GlobalReconnectThread, NULL);
    GlobalReconnectThread = 0;
  }

  if (mosq != NULL) {
    rc = mosquitto_disconnect(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
      if (rc == MOSQ_ERR_INVAL) {
        snprintf(logMessage, sizeof(logMessage), "Unable to disconnect. input parameters were invalid");
        prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
      }
      if (rc == MOSQ_ERR_NO_CONN) {
        snprintf(logMessage, sizeof(logMessage), "Unable to disconnect. client isnt connected to a broker");
        prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
      }
    }

    rc = mosquitto_loop_stop(mosq, true);
    if (rc != MOSQ_ERR_SUCCESS) {
      snprintf(logMessage, sizeof(logMessage), "Unable to stop loop. Error code: %d", rc);
      prettyLogger(LOG_LEVEL_ERROR, "MQTT", logMessage);
    }

    mosquitto_destroy(mosq);
  }

  mosquitto_lib_cleanup();

  pthread_mutex_destroy(&GlobalMQTTConnectedLock);
    
  prettyLogger(LOG_LEVEL_INFO, "MQTT", "Closed.");
}